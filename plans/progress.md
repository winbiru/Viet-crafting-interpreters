# Tiến độ phát triển V++

> Cập nhật: 13/09/2026
>
> Tỷ lệ dưới đây được tính theo số checkbox trong roadmap, chỉ dùng để theo dõi
> tiến độ đầu việc; không đại diện cho phần trăm khối lượng kỹ thuật thực tế.

## Tổng quan

| Giai đoạn | Hoàn tất | Còn lại | Tỷ lệ |
| --- | ---: | ---: | ---: |
| Ngắn hạn | 14/15 | 1 | 93% |
| Trung hạn | 18/18 | 0 | 100% |
| Dài hạn | 13/28 | 15 | 46% |
| **Tổng** | **45/61** | **16** | **74%** |

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
- [x] Parity gate hiện đối chiếu 70 chương trình `.vi`; direct IR backend bao phủ
  70/70 chương trình.
- [x] Parity gate chạy lại cùng corpus theo thứ tự đảo trong cùng process; regression
  compile A → B → A đã khóa lifecycle reset giữa nhiều lần biên dịch.
- [x] VM opcode smoke đã có branch (`OP_JUMP`, `OP_JUMP_IF_FALSE`) và call/return
  (`OP_GOI`, `OP_PARAM`, `OP_TRA_VE`) ở mức bytecode trực tiếp.
- [x] VM opcode smoke đã khóa native collection/text adapter qua direct/indirect call,
  boolean/null + unary stack và assignment/increment/decrement; runtime đã có handler
  trực tiếp cho `OP_DUNG_GIA_TRI` và `OP_SAI_GIA_TRI`.
- [x] `VM::run()` đã được thu gọn thành lifecycle/GC + opcode routing; call, value,
  index, variable/call-frame, switch/block, loop-control, exception và branch có
  handler riêng. Baseline regression hiện tại đạt 61/61.
- [x] VM opcode matrix hiện khóa arithmetic, logic/comparison boundary, stack,
  branch, call/return, native call, default parameter, switch/default, throw/catch,
  uncaught error và các lỗi boundary chính.
- [x] `VMRuntimeFixture` + `vpp-vm-handler-unit` đã tạo test boundary nội bộ cho
  stack/PC/variables/call frame/control stacks và output sink, cho phép test handler
  trực tiếp mà không phải chạy toàn bộ dispatch/stdout.
- [x] `CompilationContext` hiện sở hữu `StringPool`, function maps, import set và
  class/access state. `vpp-compiler-support-unit` có regression compile song song bằng
  hai context và xác nhận registry không rò chéo giữa hai thread.
- [x] Import resolution dùng `CompilationContext.importResolutionBase`; regression
  song song biên dịch hai `module.vi` cùng tên từ hai thư mục độc lập mà không đổi
  process cwd. CLI chỉ giữ cwd compatibility trong lúc VM thực thi file/database tương đối.
- [x] Direct emitter đã có ranh giới `emitBlock`, `emitStatement` và
  `emitFunctionBody`, làm rõ dữ liệu vào-ra của block/statement/function trước khi
  tiếp tục các refactor compiler lớn hơn.
- [x] **Module semantics Phase 1:** local `.vi` module có identity/export index,
  namespace alias đưa export vào `SemanticEnvironment`, imported function có call kind
  riêng nhưng vẫn giữ runtime-name linking, và lifecycle tracker khóa
  `uninitialized → initializing → initialized/failed`.
- [x] Quality baseline đã có coverage gate 45%, `.clang-tidy` versioned và benchmark
  lặp lại được cho VM dispatch, lexer/compiler và native HTTP helpers.

## Đang thực hiện

- [x] **Compiler state:** `CompilationContext` đã trở thành owner của mutable registry;
  `StringPool`/`hamMap` chỉ còn là facade tới context active để giữ tương thích cho
  codegen/import. CLI không còn đọc registry global sau compile; import lookup dùng
  resolution base riêng của context và concurrent compilation có import độc lập đã có
  regression.
- [x] **Bỏ token bridge:** production compiler chỉ còn Direct IR → bytecode. Import local
  dùng metadata AST/IR có cấu trúc và compile đệ quy qua pipeline hiện tại; token-dispatch
  compiler và các `compile*.cpp` cũ đã bị xóa khỏi source/CMake. Region chưa được direct
  emitter hỗ trợ giờ làm compile thất bại với diagnostic tường minh. `materializeIrTokens()`
  chỉ còn phục vụ lossless IR test/debug, không nằm trên production compile path.
- [x] **CI quality:** coverage threshold và clang-tidy chạy trên Ubuntu; macOS đã có
  job build + full CTest thường trực bên cạnh Ubuntu và Windows.
- [x] **Module runtime:** runtime có `ModuleTable` độc lập compiler; CLI chuyển module
  initializer metadata vào VM, initializer chạy dependency-first đúng một lần và giữ
  state `failed` khi khởi tạo ném lỗi. Explicit export/re-export, richer cycle diagnostic
  và package resolver vẫn còn theo roadmap.
- [x] Regression `.vi` cho module runtime đã khóa ba contract end-to-end qua CLI:
  dependency-first initialization, duplicate import chỉ khởi tạo một lần và alias
  namespace vẫn gọi được imported function sau khi initializer chạy.
- [ ] **Object model:** runtime substrate đã có `RuntimeClass`/`RuntimeInstance`, method
  table + superclass lookup, instance fields và identity equality trong `StackValue`.
  Semantic/IR/bytecode/VM hiện đã chạy end-to-end zero-arg `Class()`, field read/write
  và bound method call; `.vi` regression `kiem_tra_object_model.vi` khóa contract này.
  Còn thiếu implicit receiver, constructor có tham số, inheritance syntax và instance
  member visibility trước khi chuyển ownership sang tracing GC.

## Rủi ro cần xử lý sớm

- [x] Xác nhận tính độc lập của `vpp-pipeline-legacy-parity`: corpus 70 chương trình
  chạy thuận và đảo thứ tự trong cùng process đều khớp snapshot; chạy riêng parity
  và bộ CTest không gồm integration đều qua.
- [x] Handler VM vẫn thao tác trên state của instance, nhưng `VMRuntimeFixture` đã tạo
  ranh giới test nội bộ để dựng/quan sát state và gọi handler độc lập. Việc tối ưu
  dispatch sâu hơn giờ có baseline handler-level để bảo vệ semantics.
- [x] Import resolution đã tách khỏi process cwd cho context-driven compilation; hai
  compilation có import cùng tên từ hai working directory khác nhau chạy song song và
  giữ đúng module riêng của từng context.

## Kiểm tra tại thời điểm cập nhật

```text
VM opcode smoke (build trực tiếp bằng C++17): passed
Integration regression: 61/61 passed
CTest baseline gần nhất: 15/15 passed
Pipeline parity baseline gần nhất: 70 chương trình, direct IR 70 chương trình
Coverage cross-check: 75.77% line coverage (8,884/11,725), gate 45%
Benchmark baseline: VM dispatch + lexer + compiler pipeline + native HTTP helpers
Short-term: 14/15
Medium-term: 18/18
Long-term: 13/28
```

## Ưu tiên tiếp theo

1. [x] Làm test parity chạy độc lập ổn định và thêm regression cho lifecycle compile nhiều lần.
2. [x] Mở rộng VM opcode matrix cho branch + call/return trước khi tách `VM::run()`.
3. [x] Hoàn tất bước đầu `CompilationContext`: top-level reset/snapshot/cleanup và
   migrate `runSnippet()`; tiếp tục dời registry nội bộ ở các bước sau.
4. [x] Đưa toàn bộ regression corpus hiện tại sang direct IR, khóa bằng parity gate và
   xóa token bridge khỏi production compiler/source set.
5. [x] Sau khi test architecture ổn định, thêm coverage + clang-tidy + benchmark baseline.
6. [x] Tách nốt variable/index/switch/block/exception khỏi `VM::run()` và mở rộng
   opcode matrix; full regression hiện tại đạt 61/61.
7. [x] Tạo internal VM state fixture/API và output sink để test handler trực tiếp
   không phụ thuộc stdout; khóa bằng `vpp-vm-handler-unit`.
8. [x] Dời `StringPool`, function registry, import set và class/access state vào
   `CompilationContext`; thêm concurrent-compilation regression và migrate toàn bộ CLI
   sang context snapshot.
9. [x] Tách import resolution khỏi process cwd bằng `CompilationContext.importResolutionBase`
   và khóa bằng regression song song cho hai module cùng tên ở hai thư mục độc lập.
10. [x] Tách ranh giới direct emitter thành `emitBlock`, `emitStatement` và
    `emitFunctionBody` với output bytecode tường minh; parity 70/70 và full CTest vẫn xanh.
11. [x] Hoàn tất Module Semantics Phase 1: index local `.vi` exports, alias namespace,
    semantic `ImportedFunction`, lifecycle state contract và production top-level
    indexing; package/compat imports vẫn do resolver hiện hữu xử lý.
12. [x] Nối module lifecycle vào runtime `ModuleTable`, chạy initializer dependency-first
    đúng một lần và giữ failed state khi lỗi.
13. [ ] Hoàn tất object model: zero-arg construction, property read/write và bound-method
    dispatch đã chạy qua semantic/direct IR/VM; tiếp theo chốt implicit receiver,
    constructor có tham số, inheritance syntax + visibility trước tracing GC,
    stack trace/debugger và package resolver.
