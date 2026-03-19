# ManusGlove 프로젝트 규칙

ManusInteraction 플러그인: Manus Gloves의 kinematic hand tracking을 UE5 physics와 연결하는 플러그인.

## 핵심 문서

| 문서 | 경로 | 역할 |
|------|------|------|
| **handoff.md** | `Plugins/ManusInteraction/handoff.md` | 아키텍처, 파일 구조, 클래스 상세 설명, 알려진 한계 |
| **blueprintsetup.md** | `Plugins/ManusInteraction/blueprintsetup.md` | Blueprint 설정 가이드, 프로퍼티 테이블, 이벤트 그래프 예시 |

---

## 문서 동기화 규칙

코드 변경 시 **같은 커밋**에서 문서도 반드시 업데이트한다.

| 코드 변경 유형 | handoff.md | blueprintsetup.md |
|---------------|------------|-------------------|
| 새 컴포넌트/클래스 추가 | 파일 구조 트리 + 클래스 설명 추가 | Blueprint 설정 예시 추가 |
| UPROPERTY/UFUNCTION 추가·변경·삭제 | 해당 클래스 설명 수정 | 프로퍼티 테이블 수정 |
| 이벤트(Delegate) 추가·변경 | 해당 클래스 설명 수정 | 이벤트 그래프 예시 수정 |
| 버그 수정 (동작 변경) | 필요시 알려진 한계 섹션 수정 | 필요시 디버깅 팁 수정 |
| 파일 이름/경로 변경 | 파일 구조 트리 수정 | — |
| 단순 내부 리팩토링 (API 변경 없음) | 불필요 | 불필요 |

---

## 새 컴포넌트 문서 템플릿

### handoff.md 추가 형식

```
**클래스이름**
- 상속 구조 및 핵심 설계 결정 (왜 이 방식인지)
- 주요 UPROPERTY 목록 (타입, 기본값, 용도)
- 틱 그룹 및 실행 순서 관련 사항
- 이벤트/델리게이트 목록
- Blueprint 호출용 API (UFUNCTION)
- 엣지 케이스 또는 주의사항
```

### blueprintsetup.md 추가 형식

```
### 3-N. [용도 이름] ([컴포넌트 이름])

**Blueprint 생성:**
1. Actor Blueprint 이름
2. Components 목록

**[컴포넌트] 설정:**
| 프로퍼티 | 값 | 이유 |

**이벤트 그래프:**
[이벤트 → 게임 로직 연결 예시]
```

---

## 커밋 메시지 컨벤션

- `feat:` — 새 기능 (문서 업데이트 필수)
- `fix:` — 버그 수정 (API 변경 시 문서 업데이트)
- `docs:` — 문서만 변경
- `refactor:` — 내부 리팩토링 (API 변경 없으면 문서 불필요)

---

## Git 규칙

- **모든 작업은 `main` 브랜치에 직접 커밋한다.** 별도 feature branch를 만들지 않는다.

---

## 빌드

- 엔진: Unreal Engine 5.5
- 플랫폼: Win64 only (Manus SDK 제약)
- 플러그인 소스: `Plugins/ManusInteraction/Source/ManusInteraction/`
- Manus Core Plugin (`Plugins/Manus/`)은 원칙적으로 수정하지 않는다
- **예외 패치**: `ManusComponent.cpp:204` — `InitManusReplicatorID()`에서 `GetOwner()` null 크래시 수정 (null guard 추가). Manus 플러그인 업데이트 시 재적용 필요.
