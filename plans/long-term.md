# Kế hoạch dài hạn (3–12+ tháng)

> Cập nhật: 13/09/2026
> Các mục dưới đây là công việc nền tảng chưa hoàn tất. V++ hiện có compiler, bytecode
> VM, package/gói chuẩn, tooling MVP và pipeline release; không nên diễn giải
> điều đó là mức hoàn thiện tương đương Java, C# hay Python.

## Nền tảng đã có

- [x] Workflow release tạo artifact cho Linux, macOS và Windows, bao gồm binary,
  gói/chuẩn, template và ví dụ.
- [x] CONTRIBUTING.md đã có hướng dẫn đóng góp cơ bản.
- [x] Runtime có MVP cho GC/JIT và CI có regression/sanitizer nền tảng; các phần này
  chưa phải implementation production-grade có profiling đầy đủ.
- [x] Pipeline đã có expression AST mang span, scope tree, ExprId-based resolution,
  recursive untyped IR, optimizer và direct bytecode backend; production token bridge đã bị xóa.
- [x] Parity gate compile toàn bộ `src/tests/**/*.vi` qua pipeline rồi đối chiếu
  fingerprint bytecode, StringPool và function registries với baseline legacy
  đóng băng dưới `test/data/`.

## Việc lớn còn lại

1. Frontend và semantic pipeline

   Thứ tự migration bắt buộc, mỗi bước phải giữ regression bytecode/VM của toàn bộ
   corpus `.vi` trước khi chuyển bước tiếp theo:

   1. [x] **Expression AST** — parse literal, name, unary/binary, assignment,
      call, lambda và map theo precedence hiện hành; vẫn giữ source span/token
      range làm fallback tạm thời.
   2. [x] **Scope tree** — tạo global/class/function/block/lambda/catch scope,
      parent/child link và khai báo parameter/local/import alias.
   3. [x] **Real name resolution** — bind expression node tới symbol lexical,
      phân biệt direct call, indirect value call và dynamic/native fallback.
   4. [x] **Recursive IR lowering** — hạ expression arena, parameter defaults và
      statement children; conditional/loop/switch/try/class đã có structured payload;
      import local/package đã có structured `AstImportSpec`/IR payload và module graph.
   5. [x] **IR → bytecode trực tiếp, cohort đầu** — emitter đã phát trực tiếp
      literal/name/operator/assignment/postfix/print/primitive map, top-level function,
      primitive default, return, direct/dynamic/indirect calls, structured lambda,
      if/else/for-loop/switch/try, continue, break, throw và class namespace/method.
      Emitter tự sở hữu function ID, VM slot và jump
      fixup; shared mixed-backend context còn thiếu.
   6. [x] **Bỏ token bridge** — production compiler chỉ còn Direct IR → bytecode;
      vùng chưa được direct emitter hỗ trợ bị từ chối tường minh. Corpus `.vi`
      đạt 70/70 direct IR và vẫn giữ cùng bytecode/compiler state.

   Hiện direct backend bao phủ 70/70 regression program; parity gate yêu cầu toàn bộ
   corpus phải giữ direct IR và vẫn đối chiếu đầy đủ bytecode/StringPool/function
   registries. Call argument, function/lambda parameter và loop header đã dùng chung
   splitter top-level quote-aware; string chứa delimiter không còn tự tạo fallback.
   Helper loop cũ và implementation `compileFunction` không còn caller đã được xóa.
   `materializeIrTokens()` chỉ còn phục vụ lossless IR test/debug; nó không còn nằm
   trên production compile path. Các cú pháp ngoài direct cohort được diagnostic.

   Chốt dynamic/static/gradual typing và Typed IR là quyết định riêng. Expression
   AST, scope tree, name resolution và structured **untyped IR** không phải chờ
   quyết định kiểu này.

2. Module semantics

   - [x] **Phase 1: identity + export/import namespace + lifecycle contract** — local
     `.vi` module có identity ổn định, export index cho top-level function/class
     public hoặc không ghi visibility, alias import được đưa vào `SemanticEnvironment`,
     call tới imported function có semantic kind riêng nhưng vẫn dùng runtime-name
     linking để giữ bytecode tương thích. `ModuleInitializationTracker` khóa trạng thái
     `uninitialized → initializing → initialized/failed` và chặn begin lặp/cycle.
   - [x] Nối lifecycle vào runtime `ModuleTable`: compiler giữ initializer metadata theo
     module identity, CLI chuyển metadata sang VM, và runtime chạy initializer theo thứ
     tự dependency-first đúng một lần với state `uninitialized → initializing →
     initialized/failed`.
   - [ ] Chốt cycle behavior giàu diagnostic hơn, explicit export/re-export và diagnostic
     khi truy cập symbol không export.

3. Runtime, object model và debugger

   - [ ] Hoàn tất value/object/instance semantics ở mức ngôn ngữ. Runtime substrate đã
     có `RuntimeClass`/`RuntimeInstance`, field storage, method table, inheritance lookup
     và identity equality. Lát cắt đầu tiên đã chạy end-to-end qua semantic → untyped IR
     → bytecode → VM: `obj = Class()`, `obj.field = value`, `obj.field` và
     `obj.method(args)`. Còn phải chốt implicit receiver (`this`/`self`), constructor có
     tham số, inheritance syntax + visibility của instance member trước khi coi object
     model hoàn chỉnh. Trait/generic tiếp tục phụ thuộc quyết định ngôn ngữ.
   - [ ] Nâng MVP GC thành tracing GC quản lý object graph an toàn, với root từ VM
     stack/call frame/global/module table và regression cycle/ownership.
   - [ ] Xây stack trace có source span/function/module identity và debugger hook cho
     breakpoint, step, frame/variable inspection trước khi làm debugger UI đầy đủ.
   - [ ] Chỉ tối ưu JIT/dispatch sau benchmark; cần fallback interpreter và regression
     cross-platform.
   - [ ] Thiết kế concurrency/async và native/FFI với ownership/cancellation rõ ràng.

4. Package resolver

   - [ ] Tách package/bare-module lookup khỏi `compileRegistry`: project package,
     bundled stdlib, compatibility redirect và `VPP_HOME` phải đi qua resolver có
     identity/dependency policy rõ ràng trước khi thêm version/lockfile.

5. Chuẩn hoá bytecode

   - [ ] Định nghĩa format, versioning, validation và compatibility policy sau khi
     opcode ổn định.
   - [ ] Viết assembler/disassembler và round-trip test; tránh coi tài liệu proposal
     là format runtime đã phát hành.

6. API embedding

   - [ ] Thiết kế C API/C++ API cho lifecycle VM, load/chạy program và callback I/O.
   - [ ] Loại dependency vào global compiler state trước khi công bố API ổn định.
   - [ ] Thêm ABI/versioning, sample host và test API độc lập.

7. Phát hành và cộng đồng

   - [ ] Duy trì workflow release hiện có, thêm smoke test artifact cài từ package nếu
     cần.
   - [ ] Đánh giá Homebrew, Debian/MSI hoặc package manager khác sau khi install
     contract ổn định; chúng chưa có trong repo.
   - [ ] Bổ sung CODE_OF_CONDUCT, issue/PR template, changelog/release-note process
     và maintainer guide. CONTRIBUTING.md đã hoàn thành phần đầu tiên.

## Điều kiện thực hiện

- Không bắt đầu Typed IR hay public embedding API trước khi quyết định type policy và
  lifecycle dữ liệu; điều kiện này không chặn sáu bước migration untyped ở trên.
- Mọi tối ưu runtime phải có benchmark, test lỗi và regression cross-platform.
- Roadmap cần được cập nhật cùng code để trạng thái checklist không bị nhầm với mức
  hoàn thiện của các platform trưởng thành.
