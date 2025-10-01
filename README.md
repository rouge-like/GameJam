# Fusion XR Classroom

몰입형 혼합현실 교실을 목표로 한 Unreal Engine 5.6 프로토타입입니다. 손 제스처, 음성 AI, 인터랙티브 위젯을 결합해 “학습자가 직접 XR 교실의 도구를 조작한다”는 경험을 실험했습니다. 이 문서는 프로젝트 방향성과 기술들을 소개합니다.

**개발 기간 Development Period**: 2025.09.25~2025.09.29

## 프로젝트 스냅샷 Project Snapshot
- **프로젝트 유형**: Game Jam 기반 MR Classroom 프로토타입 (Windows + XR)
- **핵심 목표 Goal**: 손 제스처만으로 수업 콘텐츠를 탐색하고, AI 설명·음성 응답으로 학습 몰입도를 높이는 XR 인터페이스 검증

## 비전과 컨셉 Vision & Concept
- 교사가 아닌 학습자가 주체가 되는 혼합현실 교실 경험을 만들고자 했습니다.
- MediaPipe 손 추적 데이터를 Unreal 위젯과 연결해 실제 스크린을 터치하는 듯한 감각을 구현했습니다.
- 텍스트 객체 설명과 음성 답변을 동시에 제공해 “실시간 XR 튜터” 시나리오를 구축했습니다.

## 주요 기여 Highlights
- **AFusionMode 오케스트레이션**: WebSocket 제스처 스트림과 REST 기반 설명·음성 API를 GameMode에서 통합 관리하고, 블루프린트 멀티캐스트 이벤트로 노출했습니다.
- **HandViewportMapperComponent**: 손끝 좌표를 캘리브레이션된 화면 공간으로 사영하고, 제스처 방향 기반 위젯 히트 테스트를 구현했습니다.
- **XR UI 워크플로**: `OnGestureFrameReceived`, `OnVoiceAnswerReceived`, `OnBackRequested` 델리게이트 중심으로 UX를 빠르게 반복할 수 있는 블루프린트 파이프라인을 설계했습니다.

## 기술 구조 Technical Breakdown
- **Gesture Pipeline**: MediaPipe → WebSocket → `FFusionHandSnapshot` 형태로 정규화 후 틱마다 브로드캐스트.
- **AI Services**: REST 엔드포인트 호출 후 응답 데이터를 블루프린트 이벤트로 전달했습니다.
- **Calibration System**: `FFusionScreenQuad`, `SetSourceCorner`, `AutoSetTargetQuadFromViewport`를 활용해 현실 스크린과 가상 UI를 정렬했습니다.
- **모듈러 아키텍처**: C++ 레이어는 네트워크·파싱·로그를 담당하고, 블루프린트는 UX 상태 머신과 피드백 루프를 구성하도록 분리했습니다.

## 도전과 해결 Journey & Challenges
- **제스처 잡음**: 손 추적 jitter를 보간하고 keep-alive 타이머를 추가해 WebSocket 끊김 시 빠르게 재접속하도록 설계했습니다.
- **UI 정렬**: 현실 스크린 위치가 바뀔 때마다 재보정해야 했기에, 네 코너를 한 번 지정하면 전체 히트 테스트가 재계산되는 퀵 캘리브레이션 툴을 제작했습니다.
- **AI 응답 지연**: REST 호출 지연을 사용자에게 알리기 위해 HUD 토스트와 컬러 코드 로그를 추가해 진행 상태를 전달했습니다.

## 성과와 다음 단계 Outcomes & Next Steps
- Game Jam 심사에서 “학습 상황에 바로 적용 가능한 XR UX”라는 피드백을 받았습니다.
- 후속 과제: 학습 시나리오 확장, 제스처 레코딩 데이터셋 관리, 즐겨찾기 제스처(북마크) 설계.
- 포스트잇 UX 프로토타입과 플레이터스트 로그를 바탕으로 고해상도 캡처·데모 영상을 제작 중입니다.

## 사용 기술 Tech Stack
- **엔진**: Unreal Engine 5.6 (C++/Blueprint Hybrid)
- **핵심 모듈**: `AFusionMode`, `UHandViewportMapperComponent`, `FFusionHandSnapshot`
- **AI & 툴링**: MediaPipe Gesture Stream, Python 기반 agent 서버 스텁, REST/TTS

## Demo Video

