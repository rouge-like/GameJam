# Repository Guidelines

## 프로젝트 구조 및 모듈 구성
- `Source/Fusion`: 주요 Unreal C++ 모듈(진입점은 `Fusion.cpp`, `Fusion.h`). 엔진 연동 로직을 확장할 때 이곳을 수정하세요.
- `Content/`: Unreal 에셋과 XR 교실 콘텐츠. `XR_Textbook_L1`처럼 기능 중심의 네이밍을 유지합니다.
- `Config/`: 입력, 네트워크, 렌더링 설정을 담은 기본 *.ini 파일. Unreal Editor로 편집하되 커밋 전에 변경 사항을 검토하세요.
- `Agent.md`, `AGENTS.md`: MediaPipe ↔ Unreal 연동 규약과 협업 가이드. 스키마 변경 시 두 문서를 함께 업데이트합니다.

## 빌드, 테스트, 개발 명령어
- `"<UE_PATH>\Engine\Binaries\Win64\UnrealEditor.exe" Fusion.uproject` – PIE 테스트와 블루프린트 수정을 위한 프로젝트 실행.
- `"<UE_PATH>\Engine\Build\BatchFiles\Build.bat" Fusion Win64 Development` – Windows용 C++ 모듈 컴파일.
- `"<UE_PATH>\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="$(Resolve-Path Fusion.uproject)" -noP4 -platform=Win64 -clientconfig=Shipping -build` – 배포 가능한 빌드 패키징. 스위치는 릴리스 담당자와 조율하세요.
- `python -m agent.server` (AI 프로토타입) – 제스처 서비스 스텁 구동. 스키마 변경 시 `Agent.md`와 동기화해 검증하세요.

## 코딩 스타일 및 네이밍 규칙
- Unreal 표준 준수: 4칸 들여쓰기, 클래스는 PascalCase와 `AFusion`/`UFusion` 접두사, 지역 변수는 camelCase, 열거형은 `E` 접두사.
- 블루프린트 에셋은 `BP_` 접두사와 기능을 설명하는 접미사 사용(`BP_XRTextbookController`). 파일명과 에셋명을 일치시킵니다.
- Config 키와 API 페이로드 필드는 snake_case(`page_turn`, `gesture_stream`)를 유지하고, 새 라우트는 `Agent.md`에 문서화합니다.

## 테스트 가이드라인
- Unreal Automation 테스트는 `Source/Fusion/Tests` 아래에 기능별로 추가 (`Fusion.HandControls`).
- 로컬 실행: `UnrealEditor.exe Fusion.uproject -ExecCmds="Automation RunTests Fusion.*; Quit"` 또는 Session Frontend 활용.
- AI 서비스는 pytest와 REST/WebSocket 계약 테스트를 함께 작성하고, 제스처 녹화 샘플을 회귀 검증용으로 보관하세요.

## 커밋 및 PR 가이드라인
- 명령형 커밋 메시지 사용 (`feat: add pinch-to-highlight command`) 후 자잘한 수정은 스쿼시.
- PR에는 게임플레이 영향, AI 변경 사항, 테스트 근거(Automation 로그, 수동 GIF, curl 출력)를 포함하세요.
- 이슈 링크, 설정/스키마 변경 사항을 명시하고, 스택을 넘나드는 변경은 Unreal과 AI 담당자 모두에게 리뷰를 요청하세요.

## 에이전트 연동 참고 사항
- MediaPipe → Unreal 스키마 호환성을 유지하고, 페이로드 버전 변경 시 소비자 설정을 동시에 갱신하세요.
- WebSocket/REST 토큰과 같은 민감 정보는 git에서 제외된 환경 파일에 저장하고 설정 변수로 참조하세요.
