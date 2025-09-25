# CODEX 프로젝트 현황 노트

## 현재 프로젝트 요약
- **목표**: MediaPipe로 손끝 좌표와 제스처(`point`, `back`, `next`)를 추출해 Unreal Engine 3D 교과서와 연동, LLM/TTS 기반 자동 설명까지 제공하는 XR 수업 워크플로우 구현.
- **AI 측 진행 상황**: 제스처 클래스와 confidence 정책 초안 수립, 손끝 좌표 → REST/WebSocket 전송 구조 설계, LLM/TTS 파이프라인 요구사항 정의.
- **Unreal 측 진행 상황**: `Fusion` 모듈 기초와 3D 교과서 씬 골격 구축, 좌표 기반 레이캐스트로 대상 식별 후 서버에 피드백 보내는 Listener 설계 중.
- **연동 상태**: 메시지 스키마와 명령 흐름은 `Agent.md`에 정리되어 있으며, 실제 API·이벤트 바인딩은 프로토타입 전 단계.
- **리스크**: 좌표 → 월드 매핑 오차, WebSocket 지연, LLM/TTS 응답 시간, 오검출 시 UI 오작동.

## 4일 일정 (AI / Unreal 분리)
- **Day 1**
  - *AI*: MediaPipe 파이프라인 프로토타입, 손끝 좌표 추출 안정화, 제스처 레이블 및 confidence 임계값 검증.
  - *Unreal*: 좌표 수신용 WebSocket 스텁 구성, 월드 공간 레이캐스트/히트 테스트 유틸 초기 구현.
- **Day 2**
  - *AI*: FastAPI 기반 WebSocket + REST 엔드포인트 구현(`POST /context/describe` 등), 좌표→객체 ID 요청 응답 프로토타입.
  - *Unreal*: 식별한 객체 ID를 AI 서버로 전달하는 REST 호출 작성, 더미 응답으로 하이라이트/줌 핸들러 테스트.
- **Day 3**
  - *AI*: 객체 ID 입력 시 LLM 설명 + TTS 오디오 생성 흐름 연결, 응답 메시지 스키마(`description`, `tts_url`, `action`) 확정.
  - *Unreal*: 실제 AI 응답을 받아 텍스트 표시, 오디오 재생, 강조 연출을 통합하고 로깅/에러 핸들링 추가.
- **Day 4**
  - *AI*: 실시간 스트림 부하 및 실패 시나리오(네트워크, LLM 지연) 테스트, 재시도/타임아웃 정책 정리.
  - *Unreal*: 전 구간 통합 리허설, 좌표 오차/Offset 보정, 사용자 피드백 기반 다음 스프린트 과제 도출.
