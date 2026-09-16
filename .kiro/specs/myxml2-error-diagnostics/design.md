# 设计文档

## 目标

在**不改变正常路径输出**的前提下，把 `myxml2.cpp` 里 60 余处 `assert(0)` 的"裸行号"失败，改造成带上下文的可读诊断，并修正退出码/交互行为。对应需求 1、2、3。

## 1. 错误上报骨架

### 1.1 全局上下文

```c
static const char *g_xlsxFile = "";    // 当前处理的 xlsx（main 开头设置）
static const char *g_sheetName = "";   // 当前标签页名（getStr intern 后的常量串）
static const char *g_sheetPath = "";   // 当前 xl/worksheets/sheetN.xml
static char g_sheetPathBuf[64];        // sheetPath 的存储（避免指向退栈的局部 buf）
```

设置点：
- `main()` 开头：`g_xlsxFile = argv[1]`
- 标签页循环内、`fileStr()` 之前：`g_sheetName` / `g_sheetPath`
- `getVvs()` 返回后立刻 `g_sheetPath = ""`（后续生成阶段该路径已无意义，且防止悬垂）
- 生成/check 输出循环内：`g_sheetName = workbook[i]`

`g_sheetPath` 使用静态缓冲而非指向循环内 `char buf[64]`，是因为 `fillRow/fillSheet` 阶段（`getVvs` 之后）仍可能报错。

### 1.2 统一出口

```c
static void myxml2Error(const char *file, int line, const char *func,
                        const char *fmt, ...);
```

行为：`fflush(stdout)` → 向 stderr 打印

```
[myxml2 配置错误] 文件:fanli.xlsx 标签页:Sheet1(xl/worksheets/sheet8.xml)
  原因: 有效行数只有 0 行，至少需要 2 行(第 1 行字段类型、第 2 行字段名)；空标签页请从 Excel 中删除
  位置: myxml2.cpp:802 in getVvs
```

随后：`isatty(fileno(stdin))` 为真时打印提示并 `getchar()`（Windows 双击 `.bat` 看报错），最后 `exit(1)`。
Windows 下 `isatty/fileno` 经 `#ifdef _WIN32` 映射为 `_isatty/_fileno`。

### 1.3 宏

| 宏 | 语义 |
| --- | --- |
| `assert(exp)` | 保留占位，失败时报告 `内部断言失败: <exp 文本>`（仅剩解压内部约束等不可能由表内容触发的点） |
| `MYXML2_FAIL(fmt, ...)` | 直接报错退出，用于"已经判定为错误"的检查点 |
| `require(exp, fmt, ...)` | 条件不成立时报错退出，用于 zip 头等条件表达式 |

## 2. 检查点的专项消息

把原先只有 `assert(0)` 的检查点按语义改写（均带数字/名称上下文）：

| 位置 | 触发条件 | 现在的消息（要点） |
| --- | --- | --- |
| `getVvs` | 单元格行号回退 | 单元格[x]所在行 N 小于上一行 M，行号必须递增 |
| `getVvs` | `*n < 2` | 有效行数只有 N 行，至少需要 2 行；空标签页请从 Excel 中删除 |
| `getVvs` | 第一列 ID 重复 | 第一列存在重复的ID[x] |
| `getVvs` | 字段(列)名重复 | 存在重复的字段(列)名[x] |
| `getVvs` | 行/列/数据超上限、内部缓存不足 | 标签页行数/列数/数据超出上限 N |
| `fillRow` | 字段类型不在 number/plain/string/table/tbstr | 字段[x]的类型[y]非法，只能是 … |
| `removeSpace` | table 字段 `[[` 嵌套或 `]]` 多余 | table 字段里出现嵌套的[[ / 多余的]]，Lua 表格式错误: <内容> |
| `getStrAddQuote` | 单元格转换缓存不足 | 单元格内容过长，超出 N 字节转换缓存 |
| `fileStr` | 缺少 `xl/...` 条目 / 解压失败 / 压缩方式不支持 | xlsx 内缺少 X，文件不是标准的 xlsx 或已损坏 等 |
| `tinfUncompress` | 压缩块类型非法 / 校验失败 | xlsx 压缩数据块类型非法，文件可能已损坏 |
| `getXml` | XML 结构异常 / 节点、属性超限 | 表格 XML 结构异常 / 超出上限 N |
| `main` | zip 头版本、标志、压缩方式异常 | 不是有效的 xlsx(zip 版本 0x…) |
| `main` | 输入文件不存在 / 输出不可写 | 找不到输入文件[x] / 无法写入输出文件[x] |
| `myFwrite`, `binInsert` | 输出缓存、共享缓存不足 | 生成内容超出输出缓存上限，请拆分这个表 / 共享(share)缓存不足 |

`getVvs` 的 `filename`/`sheetname` 参数不再用于拼消息（改由全局上下文提供），保留签名不变以免波及调用点。

## 3. 配套脚本

`catd/tbls/gen.sh` 原先失败后继续处理下一张表（`all.sh` 已写 `./catd/tbls/gen.sh || exit 1`，却因为内层没有 `|| exit 1` 而形同虚设），补上 `|| exit 1`，与 `check.sh` 一致。

## 4. 验证方法

1. **正常路径零差异**：用改动前后的 myxml2 分别对 `equipZhuHun.xlsx`、`barTask.xlsx`、`buy.xlsx` 跑 `noshare _cat` 生成，`cmp` 输出文件必须一致。
2. **全量回归**：在 `catd/tbls` 副本上对全部 xlsx 逐个跑 `noshare check`，除已知的 `fanli.xlsx`（空标签页 `Sheet1`）外应全部通过，且 `fanli.xlsx` 必须给出"有效行数只有 0 行 … 标签页:Sheet1"的可读报错。
3. **坏表用例**：用最小手写 xlsx 覆盖各错误分支（空标签页、重复 ID、重复字段名、非法类型、table 格式错误、行号乱序、非 zip），核对待打印的消息与退出码 1。
4. **平台**：Linux 下 `g++ -g -Wall -march=x86-64` 编译通过；Windows 分支仅做 `#ifdef` 静态检查（本机无法构建 MSVC 产物）。
