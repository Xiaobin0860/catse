# 需求文档

## 简介

`catd/tbls` 的配置流水线（`check.sh` → `gen.sh` → 顶层 `all.sh`）依赖 `catse/myxml2` 编译出的 `myxml2` 把 Excel(`.xlsx`) 转换成 Lua 配置表。改动前，myxml2 的所有检查失败都退化成同一行裸断言输出：

```
#exp=0, __FILE__=/home/lixiaobintvs/cat/catse/myxml2/myxml2.cpp, __LINE__=743
```

既看不出**是哪张表、哪个标签页**，也看不出**原因**；而且自定义的 `assert` 宏以 `getchar()` + `exit(0)` 收尾，导致 `check.sh` 里的 `|| exit 1` 永远不生效（错误被人为回车"放行"，脚本继续跑下一张表），`all.sh` 也无法拦截坏配置。

实际触发案例：`catd/tbls/fanli.xlsx` 的第 8 个标签页 `Sheet1` 是空白页（`xl/worksheets/sheet8.xml` 无任何 `<row>`），`getVvs()` 中 `*n < 2` 失败，用户只看到一行 `#exp=0 ... __LINE__=743`。

本次改动让 myxml2 在任何表配置错误时输出**可定位的诊断信息**，并以非 0 退出码结束。

## 术语表

- **myxml2**: 由 `catse/myxml2/myxml2.cpp` 编译出的 Excel → Lua 配置转换工具，部署在 `catd/tbls/myxml2`
- **标签页**: xlsx 中的一个 sheet，对应 `xl/workbook.xml` 里的 `<sheet name="..."/>`，源码为 `xl/worksheets/sheetN.xml`
- **表约定**: 每个标签页第 1 行为字段类型、第 2 行为字段名、第 3 行为中文注释（被丢弃）、第 4 行起为数据
- **配置错误**: 表结构或内容不满足表约定的情况，如空标签页/有效行不足、行号乱序、第一列 ID 重复、字段(列)名重复、字段类型非法、table 字段 `[[`/`]]` 不配对、xlsx 不是标准 zip/缺少内部条目
- **诊断信息**: 报错时打印到 stderr 的「文件 / 标签页 / 原因 / 源码位置」四元组

## 需求

### 需求 1：错误信息可定位

**用户故事：** 作为维护配置表的人，`check.sh` 报错时我希望一眼看出是哪张表、哪个标签页、什么问题，以便直接打开 Excel 修改。

#### 验收标准

1. WHEN myxml2 命中配置错误，THE myxml2 SHALL 向 stderr 输出诊断信息，包含当前处理**文件名**、**标签页名**、**原因**
2. THE myxml2 SHALL 在诊断信息中输出出错点的源码位置（`文件名:行号 in 函数名`）
3. WHEN 错误发生在解析某个标签页的 XML 阶段，THE myxml2 SHALL 在诊断信息中额外给出该标签页对应的 `xl/worksheets/sheetN.xml` 路径
4. THE myxml2 SHALL 用中文描述可预期的配置错误，至少覆盖：有效行数不足/空标签页、行号乱序、第一列 ID 重复、字段(列)名重复、字段类型非法、table 字段 `[[`/`]]` 不配对、xlsx 不是标准 zip 或缺少内部条目
5. WHERE 错误原因中带有可量化的上下文（行号、列数、重复的 ID/列名、非法类型名等），THE myxml2 SHALL 在原因文本中带上该上下文

### 需求 2：退出码与流水线联动

**用户故事：** 作为部署者，我希望坏配置能让流水线停下来，而不是被静默跳过。

#### 验收标准

1. WHEN myxml2 命中配置错误，THE myxml2 SHALL 以非 0 退出码结束，使 `check.sh` / `gen.sh` / `all.sh` 的 `|| exit 1` 生效
2. WHILE stdin 不是终端（脚本、CI、重定向输入）时，THE myxml2 SHALL NOT 等待用户输入
3. WHILE stdin 是终端（如 Windows 下双击 `myxml2.bat`）时，THE myxml2 SHALL 保留"按回车退出"的行为，便于查看报错
4. THE gen.sh SHALL 在单张表转换失败时立即以非 0 退出，避免生成残缺配置后被 `all.sh` 提交

### 需求 3：不改变正常路径行为

#### 验收标准

1. WHEN 表配置合法，THE myxml2 SHALL 生成与改动前**逐字节一致**的 Lua 配置
2. THE myxml2 SHALL 保持既有的列过滤、行过滤、shareRow/shareCell/shareSheet 复用语义不变
3. THE myxml2 SHALL 继续支持 Linux(GCC) 与 Windows(MSVC) 编译（新增的 `isatty` 用法需在 Windows 下映射为 `_isatty`）
