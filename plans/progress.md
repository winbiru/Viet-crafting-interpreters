# Tiến độ phát triển V++

> Cập nhật: 13/09/2026
>
> Tỷ lệ dưới đây được tính theo số checkbox trong roadmap, chỉ dùng để theo dõi
> tiến độ đầu việc; không đại diện cho phần trăm khối lượng kỹ thuật thực tế.

## Tổng quan

| Giai đoạn | Hoàn tất | Còn lại | Tỷ lệ |
| --- | ---: | ---: | ---: |
| Ngắn hạn | 14/15 | 1 | 93% |
| Trung hạn | 15/18 | 3 | 83% |
| Dài hạn | 11/23 | 12 | 48% |
| **Tổng** | **40/56** | **16** | **71%** |

## Đã xác nhận hoàn thành

- [x] CMake/CTest đã tách các target core, frontend, bytecode, compiler, runtime,
  tooling, CLI và các C++ unit test.
- [x] Regression V++ được nối vào CTest trên Unix/Windows; CI Ubuntu có sanitizer.
- [x] `docs/bytecode.md` đã phân biệt rõ proposal `.vbc` với runtime contract hiện tại
  (`Instruction`/`Opcode`) và chỉ ra implementation đang thực thi.
- [x] Pipeline đã có Expression AST, scope tree, name resolution, recursive IR và
  direct IR backend cohort đầu.
- [x] Có regression cho lỗi parser/compiler/semantic, không chỉ expected-output fixture.
- [x] Test discovery đã phân biệt `test/` cho C++ unit và `src/tests/` cho regression
  V++; HTTP fixture được khởi động riêng trong integration runner.
- [x] Parity gate hiện đối chiếu 61 chương trình `.vi`; direct IR backend bao phủ
  61/61 chương trình.
- [x] Parity gate chạy lại cùng corpus theo thứ tự đảo trong cùng process; regression
  compile A → B → A đã khóa lifecycle reset giữa nhiều lần biên dịch.
- [x] VM opcode smoke đã có branch (`OP_JUMP`, `OP_JUMP_IF_FALSE`) và call/return
  (`OP_GOI`, `OP_PARAM`, `OP_TRA_VE`) ở mức bytecode trực tiếp.
- [x] VM opcode smoke đã khóa native collection/text adapter qua direct/indirect call,
  boolean/null + unary stack và assignment/increment/decrement; runtime đã có handler
  trực tiếp cho `OP_DUNG_GIA_TRI` và `OP_SAI_GIA_TRI`.
- [x] `VM::run()` đã được thu gọn thành lifecycle/GC + opcode routing; call, value,
  index, variable/call-frame, switch/block, loop-control, exception và branch có
  handler riêng. Regression trước/sau refactor đều giữ 54/54.
- [x] VM opcode matrix hiện khóa arithmetic, logic/comparison boundary, stack,
  branch, call/return, native call, default parameter, switch/default, throw/catch,
  uncaught error và các lỗi boundary chính.
- [x] `VMRuntimeFixture` + `vpp-vm-handler-unit` đã tạo test boundary nội bộ cho
  stack/PC/variables/call frame/control stacks và output sink, cho phép test handler
  trực tiếp mà không phải chạy toàn bộ dispatch/stdout.
- [x] Quality baseline đã có coverage gate 45%, `.clang-tidy` versioned và benchmark
  lặp lại được cho VM dispatch, lexer/compiler và native HTTP helpers.

## Đang thực hiện

- [ ] **Compiler state:** đã có `CompilationContext` cho top-level compile theo cơ chế
  reset + snapshot + cleanup, và CLI `runSnippet()` đã dùng context này. Nội bộ
  `StringPool`/`hamMap` vẫn là mutable global state nên compiler chưa re-entrant.
- [x] **Bỏ token bridge:** production compiler chỉ còn Direct IR → bytecode. Import local
  dùng metadata AST/IR có cấu trúc và compile đệ quy qua pipeline hiện tại; token-dispatch
  compiler và các `compile*.cpp` cũ đã bị xóa khỏi source/CMake. Region chưa được direct
  emitter hỗ trợ giờ làm compile thất bại với diagnostic tường minh. `materializeIrTokens()`
  chỉ còn phục vụ lossless IR test/debug, không nằm trên production compile path.
- [x] **CI quality:** coverage threshold và clang-tidy chạy trên Ubuntu; macOS đã có
  job build + full CTest thường trực bên cạnh Ubuntu và Windows.

## Rủi ro cần xử lý sớm

- [x] Xác nhận tính độc lập của `vpp-pipeline-legacy-parity`: corpus 61 chương trình
  chạy thuận và đảo thứ tự trong cùng process đều khớp snapshot; chạy riêng parity
  và bộ CTest không gồm integration đều qua.
- [x] Handler VM vẫn thao tác trên state của instance, nhưng `VMRuntimeFixture` đã tạo
  ranh giới test nội bộ để dựng/quan sát state và gọi handler độc lập. Việc tối ưu
  dispatch sâu hơn giờ có baseline handler-level để bảo vệ semantics.
- [ ] Global compiler registries vẫn chặn mục tiêu re-entrant/concurrent compilation.

## Kiểm tra tại thời điểm cập nhật

```text
VM opcode smoke (build trực tiếp bằng C++17): passed
Integration regression: 54/54 passed
CTest baseline gần nhất: 15/15 passed
Pipeline parity baseline gần nhất: 61 chương trình, direct IR 61 chương trình
Coverage cross-check: 75.77% line coverage (8,884/11,725), gate 45%
Benchmark baseline: VM dispatch + lexer + compiler pipeline + native HTTP helpers
Short-term: 14/15
Medium-term: 15/18
Long-term: 11/23
```

## Ưu tiên tiếp theo

1. [x] Làm test parity chạy độc lập ổn định và thêm regression cho lifecycle compile nhiều lần.
2. [x] Mở rộng VM opcode matrix cho branch + call/return trước khi tách `VM::run()`.
3. [x] Hoàn tất bước đầu `CompilationContext`: top-level reset/snapshot/cleanup và
   migrate `runSnippet()`; tiếp tục dời registry nội bộ ở các bước sau.
4. [x] Đưa toàn bộ regression corpus 61/61 sang direct IR, khóa bằng parity gate và
   xóa token bridge khỏi production compiler/source set.
5. [x] Sau khi test architecture ổn định, thêm coverage + clang-tidy + benchmark baseline.
6. [x] Tách nốt variable/index/switch/block/exception khỏi `VM::run()` và mở rộng
   opcode matrix; full regression sau refactor vẫn 54/54.
7. [x] Tạo internal VM state fixture/API và output sink để test handler trực tiếp
   không phụ thuộc stdout; khóa bằng `vpp-vm-handler-unit`.
8. [ ] Tiếp tục dời `StringPool`/function registry khỏi global compiler state và thêm
   concurrent-compilation regression trước khi tuyên bố compiler re-entrant.
