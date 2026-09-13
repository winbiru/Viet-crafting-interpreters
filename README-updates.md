# V++ — Trạng thái repo và hướng cập nhật

> Cập nhật: 13/09/2026
> Tài liệu này mô tả những gì đang có trong source tree và các hạng mục còn lại. Sự
> tồn tại của workflow không đồng nghĩa mọi lần chạy CI đều đã thành công; trạng thái
> từng lần chạy cần xem trên GitHub Actions.

## Mục đích

- Giúp contributor định vị đúng module, test và script hiện hành.
- Phân biệt phần đã có trong repo với roadmap còn dang dở.
- Dẫn đến kế hoạch ngắn, trung và dài hạn trong thư mục plans/.

## Những nền tảng đã có

| Hạng mục | Trạng thái hiện tại | Vị trí |
| --- | --- | --- |
| CI hồi quy | Workflow có Ubuntu full CTest + ASan/UBSan, Windows Release full CTest và macOS full CTest. | .github/workflows/c-cpp.yml |
| CI chất lượng | Ubuntu chạy clang-tidy, coverage gate 45% và benchmark baseline quan sát. | .github/workflows/c-cpp.yml, scripts/quality/run-coverage.sh |
| Đóng gói release | Đã có workflow tạo binary cho Ubuntu, macOS và Windows khi publish Release hoặc chạy thủ công. | .github/workflows/release-binaries.yml |
| Hướng dẫn đóng góp | Đã có hướng dẫn cơ bản cho contributor. | CONTRIBUTING.md |
| Hồi quy tích hợp | CTest gọi run_tests.sh trên Unix và scripts/windows/run-tests.ps1 trên Windows. Các chương trình V++ và output mong đợi nằm cạnh nhau. | run_tests.sh, scripts/windows/, src/tests/ |
| Test C++/tooling | CMake đăng ký compiler support, opcode/native constants, runtime value, VM opcode matrix, VM handler fixture, pipeline/module graph/recursive IR/direct codegen/parity, tooling và CLI dump checks. | test/CMakeLists.txt |
| VM handler test API | `VMRuntimeFixture` cho phép dựng stack/PC/variables/call frame/control state và gọi từng handler trực tiếp; output có sink riêng cho test. | src/include/vpp/runtime/vm_fixture.h, test/vm_handler_tests.cpp |
| Native stdlib | Runtime đã có native helpers cho chuyển kiểu/type, random, path/filesystem, environment/platform/sleep ngoài HTTP/JSON/file/config/database/collections/text. | src/runtime/native/, gói/thư viện/ |
| Vệ sinh build | Các thư mục build phổ biến, output test và binary đã được ignore; không dùng build artefact làm source. | .gitignore |

## Cấu trúc source hiện hành

- CLI: src/cli/main.cpp.
- Core và frontend: src/core/, src/frontend/; header tương ứng ở src/include/common/ và
  src/include/frontend/.
- Compiler: src/compiler/; các thành phần hỗ trợ ở src/compiler/support/ (không còn
  nằm tại src/helpers/).
- Bytecode: src/bytecode/; header theo hướng module mới ở src/include/vpp/bytecode/ và
  header tương thích cũ vẫn ở src/include/vm/.
- Runtime và native adapter: src/runtime/ và src/runtime/native/ (không còn
  src/vm/).
- Fixture test handler nội bộ: src/include/vpp/runtime/vm_fixture.h; không phải public
  embedding API.
- Tooling CLI: src/tooling/; các header theo namespace vpp đang được gom ở
  src/include/vpp/.
- Test: chương trình hồi quy V++ ở src/tests/*.vi, output ở src/tests/expected/;
  C++ unit test ở test/*.cpp. src/tests/.tmp/ chỉ là workspace tạm được tạo khi
  chạy test.
- Package/thư viện chuẩn, template và ví dụ: gói/, templates/ và examples/.

Các header trong src/include/vpp/ là hướng tổ chức API theo module; chúng chưa được
cam kết là C/C++ embedding API ổn định.

## Build và test cục bộ

Từ root của repository:

    cmake -S . -B build
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure --verbose --no-tests=error

Binary được CMake đặt trong build/bin/. Có thể chạy riêng bộ hồi quy
Unix bằng cách đặt VPP_EXEC trỏ đến binary rồi gọi run_tests.sh. Trên Windows,
CTest tự gọi PowerShell 7 và scripts/windows/run-tests.ps1 khi pwsh có mặt.

Baseline local ngày 13/09/2026: **15/15 CTest pass**, integration regression
**54/54**, direct-IR parity **61/61**.

## Việc còn lại theo thứ tự ưu tiên

1. Dời `StringPool` và function registries khỏi mutable global state, thêm regression
   cho concurrent compilation trước khi tuyên bố compiler re-entrant.
2. Chốt chính sách kiểu dữ liệu (dynamic, static hoặc gradual) trước khi thêm type
   checking/Typed IR.
3. Chốt ABI/versioning `.vbc`, rồi mới viết serializer/deserializer,
   assembler/disassembler và verifier tương ứng.
4. Thiết kế C API/C++ embedding API dựa trên lifecycle compiler/runtime đã tách state.
5. Nâng MVP GC/JIT thành thiết kế có profiling và regression cross-platform; benchmark
   baseline hiện đã có để đo trước/sau tối ưu.

Xem chi tiết và trạng thái từng nhóm ở:

- plans/short-term.md — việc có thể hoàn tất trong vòng 1–2 tuần.
- plans/medium-term.md — refactor và độ tin cậy trong 1–3 tháng.
- plans/long-term.md — nền tảng ngôn ngữ/runtime và phát hành dài hạn.
