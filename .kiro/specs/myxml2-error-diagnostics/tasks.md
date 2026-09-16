# 任务清单

## 实现

- [x] 1. `myxml2.cpp` 头部：新增 `stdarg.h`、上下文全局量、`myxml2Error()`、`MYXML2_FAIL`/`require` 宏，改写 `assert` 宏（失败时报告 `#exp` + 位置，非交互不等待，退出码 1）
- [x] 2. zip/xlsx 解析（`main`）、`fileStr`、`tinfUncompress` 的失败点补可读消息
- [x] 3. `getXml` / `getStr` / `getWorkbook` / `getSharedStrings` / `strReplace` / `getStrAddQuote` 的失败点补消息
- [x] 4. `getVvs`：行号乱序、`*n < 2`（空标签页）、重复 ID、重复列名、行/列/缓存超限补消息
- [x] 5. `fillRow`（非法字段类型）、`removeSpace`（`[[`/`]]` 不配对）、`writeInt` 补消息
- [x] 6. `catd/tbls/gen.sh` 失败即停（`|| exit 1`）
- [x] 7. 构建并部署 `catd/tbls/myxml2`

## 验证

- [x] 8. Linux 编译：`g++ -g -Wall -march=x86-64` 通过（无新增 warning 类别）
- [x] 9. 正常路径零差异：`equipZhuHun.xlsx` / `barTask.xlsx` / `buy.xlsx` 生成结果与旧二进制逐字节一致
- [x] 10. 全量 `check.sh` 语料回归：95 张表，94 通过；`fanli.xlsx` 报"标签页:Sheet1(…sheet8.xml) 有效行数只有 0 行"
- [x] 11. 坏表用例：空标签页 / 重复 ID / 重复字段名 / 非法类型 / table 格式错误 / 行号乱序 / 非 zip / 输入不存在 / 输出不可写，均给出可定位消息且退出码 1
- [x] 12. `clang-format` 保持一致（项目无 `.clang-format`，沿用 LLVM 默认风格，格式化后无无关改动）

## 遗留

- [ ] Windows 版 `catd/tbls/myxml2.exe` 需在 Windows/MSVC 环境重新编译后替换（本机无 MSVC）
- [ ] `catd/tbls/fanli.xlsx` 的空标签页 `Sheet1` 需在 Excel 中删除后重新 `check.sh`
