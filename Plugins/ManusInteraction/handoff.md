# ManusInteraction Plugin — Handoff Document

## Overview

ManusInteraction은 Manus Gloves의 kinematic hand tracking 데이터를 UE5 physics 시뮬레이션과 연결하는 플러그인이다. Manus Core Plugin 위에 별도 레이어로 동작하며, 원본 Manus Plugin 코드를 수정하지 않는다.

**핵심 문제**: Manus 글러브는 LiveLink을 통해 bone transform을 직접 덮어쓰는 kinematic 방식이라, 물리 충돌에 반응하지 않는다. 손가락이 표면을 관통하고, 물체를 물리적으로 밀거나 잡을 수 없다.

**해결**: Dual-Hand 아키텍처. Kinematic Hand(기존 Manus)에서 bone 위치를 읽고, Physics Hand(신규)의 sphere body들을 velocity drive로 해당 위치를 추종시킨다. Physics body는 UE5 물리 시뮬레이션에 참여하므로, 표면에서 멈추고 물체를 밀 수 있다.

---

## 아키텍처

```
┌─────────────────────────────────────────────────────┐
│  Manus Core Plugin (수정 안 함)                       │
│  ┌───────────────┐    ┌──────────────────────┐      │
│  │ Manus Glove HW│───>│ LiveLink → AnimPose  │      │
│  └───────────────┘    └──────────┬───────────┘      │
│                                  │                   │
│                       ┌──────────▼───────────┐      │
│                       │  UManusComponent      │      │
│                       │  (Kinematic SkMesh)   │      │
│                       └──────────┬───────────┘      │
└──────────────────────────────────┼──────────────────┘
                                   │ GetBoneTransform()
┌──────────────────────────────────┼──────────────────┐
│  ManusInteraction Plugin (이 플러그인)                │
│                       ┌──────────▼───────────┐      │
│                       │  MIHandDriver         │      │
│                       │  (Velocity Drive)     │      │
│                       └──────────┬───────────┘      │
│                                  │ SetPhysicsVelocity()
│                       ┌──────────▼───────────┐      │
│                       │  MIPhysicsHand        │      │
│                       │  ├ Thumb  (Sphere)    │      │
│                       │  ├ Index  (Sphere)    │      │
│                       │  ├ Middle (Sphere)    │      │
│                       │  ├ Ring   (Sphere)    │      │
│                       │  ├ Pinky  (Sphere)    │      │
│                       │  └ Palm   (Sphere)    │      │
│                       └──────────┬───────────┘      │
│                                  │ Collision Events   │
│            ┌─────────────────────┼────────────────┐  │
│            │                     │                │  │
│  ┌─────────▼──┐  ┌──────────────▼──┐  ┌─────────▼┐ │
│  │ Interactable│  │ HapticFeedback  │  │ PhysGrab │ │
│  │ Components  │  │ Manager         │  │ Component│ │
│  └────────────┘  └─────────────────┘  └──────────┘ │
└─────────────────────────────────────────────────────┘
```

---

## 파일 구조

```
Plugins/ManusInteraction/
├── ManusInteraction.uplugin          # 플러그인 정의
├── handoff.md                        # 이 문서
├── blueprintsetup.md                 # Blueprint 설정 가이드
├── Source/ManusInteraction/
│   ├── ManusInteraction.Build.cs     # 빌드 의존성
│   ├── Public/
│   │   ├── ManusInteraction.h        # 모듈 인터페이스, LogManusInteraction
│   │   ├── Utils/
│   │   │   └── MITypes.h             # 공통 enum, struct (EMIFingerName, FMIPhysicsHandConfig 등)
│   │   ├── Core/
│   │   │   ├── MIPhysicsHand.h       # Physics Hand Actor
│   │   │   ├── MIPhysicsFingerComponent.h  # 개별 손가락 sphere body
│   │   │   └── MIHandDriver.h        # Velocity/Force drive 로직
│   │   ├── Haptics/
│   │   │   └── MIHapticFeedbackManager.h  # 충돌→Manus haptic API
│   │   ├── Interaction/
│   │   │   ├── MIInteractableBase.h       # 인터랙터블 추상 베이스
│   │   │   ├── MIHingeInteractable.h      # 회전 (레버, 도어핸들, 선바이저)
│   │   │   ├── MIButtonInteractable.h     # 푸시 버튼 (선형 이동)
│   │   │   ├── MIDialInteractable.h       # 다이얼/노브 (회전)
│   │   │   ├── MISliderInteractable.h     # 슬라이더 (선형 범위)
│   │   │   └── MISurfaceInteractable.h    # 터치 표면 (UV 좌표)
│   │   ├── Widget/
│   │   │   └── MIWidgetSurface.h     # UMG 위젯 관통 방지
│   │   └── Grab/
│   │       ├── MIPhysicsGrabComponent.h   # 물리 Grab (런타임 constraint)
│   │       └── MIGrabbableComponent.h     # Grabbable 마킹
│   └── Private/
│       └── (위 각 헤더의 .cpp 구현 파일)
```

---

## 핵심 클래스 설명

### Core

**AMIPhysicsHand** — Physics Hand Actor
- 손 하나를 대표하는 Actor. 내부에 5개 finger sphere + 1개 palm sphere를 생성한다.
- `SourceManusComponent`를 설정하면 `InitializeHand()`에서 bone 이름을 매핑하고 physics body를 생성한다.
- `FMIPhysicsHandConfig` struct로 속도, 반경, 질량, damping 등을 일괄 설정한다.
- `bShowDebugVisualization = true`로 하면 에디터에서 sphere 위치와 kinematic target 사이의 선을 볼 수 있다.
- 기본 bone 이름은 `SetDefaultBoneNames()`에서 Manus 표준 skeleton 기준으로 설정된다. 다른 skeleton을 쓸 경우 `FingerTipBoneNames` TMap을 직접 설정해야 한다.

**UMIPhysicsFingerComponent** — 손가락 Sphere
- USphereComponent를 상속하며 `SimulatePhysics = true`, CCD 활성화.
- `OnFingerContactBegin` / `OnFingerContactEnd` 델리게이트로 충돌 이벤트를 외부에 전달한다.
- `CurrentDivergence`: kinematic target과 실제 physics body 사이의 거리 (cm). 이 값이 크면 표면에 막혀있다는 의미.
- `bIsInContact`: 현재 무언가에 닿아 있는지.
- `ContactActor`: 현재 접촉 중인 Actor (WeakObjectPtr).

**UMIHandDriver** — 드라이브 로직
- `TG_PrePhysics`에서 tick. 매 프레임:
  1. `SourceManusComponent->GetSocketTransform(BoneName)` 으로 kinematic 위치 읽기
  2. `(Target - Current) / DeltaTime` 으로 velocity 계산
  3. `MaxLinearSpeed`로 clamp
  4. `SetPhysicsLinearVelocity()` 로 physics body에 적용
- Angular velocity도 동일한 방식으로 적용.
- `DriveMode::Force` 선택 시 velocity 대신 force를 적용.
- `OnDivergenceExceeded` 이벤트로 divergence가 threshold를 넘었을 때 알림.

### Haptics

**UMIHapticFeedbackManager**
- `TG_PostPhysics`에서 tick. 각 finger의 divergence를 읽고, `MinDivergenceForHaptic` ~ `MaxDivergenceForFullHaptic` 사이에서 선형 보간하여 haptic 강도를 계산한다.
- `bHapticOnContact = true`면 접촉 즉시 `ContactHapticIntensity`로 진동.
- 초기화 시 `DoesSkeletonHaveHaptics()` 로 글러브 haptic 지원 여부를 확인. 미지원이면 아무 것도 하지 않는다.
- `TriggerFingerHaptic()` 으로 특정 finger에 수동 haptic 트리거 가능.

### Interaction

모든 인터랙터블은 `UMIInteractableBase`를 상속한다.

**공통 인터페이스:**
- `AllowedFingers`: 허용 손가락 목록 (비어있으면 전부 허용)
- `OnInteractionBegin/Update/End`: 인터랙션 이벤트
- `GetCurrentValue()`: 0.0~1.0 정규화 값
- `bEnableHapticFeedback`, `HapticStrength`: haptic 설정
- `FindPhysicsMesh()`: 소유 Actor의 첫 번째 StaticMeshComponent 자동 탐색

**MIHingeInteractable**
- 소유 Actor의 mesh에 physics를 활성화하고, `UPhysicsConstraintComponent`를 Hinge로 설정.
- `HingeAxis` (X/Y/Z), `MinAngle`, `MaxAngle` 으로 회전 범위 정의.
- `bAutoReturn`: 손을 떼면 `RestAngle`로 자동 복귀 (ReturnSpeed deg/s).
- `SnapPositions` (float array, 0~1): 스냅 위치들. `OnHingeSnapped` 이벤트 발생.
- `SetAngle()`: 프로그래밍으로 각도 직접 설정 가능.

**MIButtonInteractable**
- 선형 이동 constraint. `PressAxis`, `PressDepth`로 눌림 방향과 깊이 설정.
- `ActivationThreshold` (0~1): 이 깊이 이상이면 "pressed" 상태.
- `bIsToggle`: 토글 모드. `OnButtonToggled(bool)` 이벤트.
- `ReturnForce`: 스프링 복귀 힘. 매 틱 현재 depth에 비례하여 복귀 force 적용.

**MIDialInteractable**
- 회전 constraint. `RotationAxis`, `MinAngle`, `MaxAngle` 또는 `bInfiniteRotation`.
- `Steps`: discrete step 수 (0 = continuous). `OnDialStepped(int)` 이벤트.
- `OnDialRotated(float Angle, float NormalizedValue)`.

**MISliderInteractable**
- 선형 이동 constraint. `SlideAxis`, `SlideDistance`.
- `Steps`: discrete step 수. `OnSliderStepped(int)`.
- `OnSliderMoved(float NormalizedValue)`.

**MISurfaceInteractable**
- Physics body와 표면 사이의 거리 기반 터치 감지.
- `SurfaceExtent`, `SurfaceNormalAxis`, `TouchDetectionDistance`로 터치 영역 정의.
- `WorldToSurfaceUV()`: 3D 월드 좌표 → 2D UV (0~1) 변환.
- `OnSurfaceTouched`, `OnSurfaceMoved`, `OnSurfaceReleased` — 각각 finger name + UV + pressure 전달.
- 멀티터치 지원: `GetActiveTouchCount()`.

### Widget

**UMIWidgetSurface**
- `UWidgetComponent` 앞에 얇은 `UBoxComponent` (CollisionSurface)를 자동 생성.
- Physics finger가 이 box에 부딪혀서 물리적으로 멈춤 → 위젯 관통 방지.
- `WorldToWidgetLocal()`: 3D 월드 좌표 → 위젯 로컬 2D 좌표 변환.
- `OnWidgetTouched`, `OnWidgetTouchMoved`, `OnWidgetTouchReleased` 이벤트.
- `bSendWidgetEvents`: true면 synthetic 포인터 이벤트 전송 시도 (현재 이벤트 브로드캐스트까지 구현, 실제 Slate input injection은 프로젝트에서 추가 구현 필요).
- `RefreshCollisionSurface()`: 위젯 크기 변경 시 collision box 업데이트.

### Grab

**UMIPhysicsGrabComponent**
- `SetFingerComponents()` 로 모니터링할 finger 목록 설정.
- 매 틱 `DetectGrabCandidate()`: 각 finger의 `ContactActor`를 집계, `MinContactFingers`개 이상 + (optional) thumb 접촉 확인.
- Grab 시 런타임 `UPhysicsConstraintComponent` 생성: 6DOF + spring drive (`GrabStiffness`, `GrabDamping`).
- `BreakForce` / `BreakTorque`: constraint 파괴 임계값. 너무 강하게 당기면 자동 release.
- `ShouldMaintainGrab()`: 접촉 finger 수가 `MinContactFingers - 1` 미만이면 release (hysteresis로 떨림 방지).
- `ForceRelease()`: 수동 release.

**UMIGrabbableComponent**
- Actor에 추가하면 "잡기 가능" 마킹.
- `bUseGravityWhenReleased`, `bUseGravityWhileHeld`로 중력 제어.
- `NotifyGrabbed()` / `NotifyReleased()` 에서 중력 토글 + 이벤트 브로드캐스트.

---

## 의존성

- **Manus Core Plugin** (v3.1.0) — `UManusComponent`, `UManusBlueprintLibrary` 참조
- **UE5.5** — PhysicsCore, UMG, LiveLink
- **Win64 only** (Manus SDK 제약)

---

## 알려진 한계 및 주의사항

1. **Bone 이름 의존성**: `SetDefaultBoneNames()`의 기본값은 Manus 표준 hand skeleton 기준. 다른 skeleton 사용 시 `FingerTipBoneNames` TMap을 직접 설정해야 함.

2. **Physics sub-stepping**: 90Hz VR에서 작은 sphere body가 빠르게 움직이면 얇은 충돌면을 관통할 수 있음. CCD가 활성화되어 있지만, 프로젝트 설정에서 physics sub-stepping을 활성화하면 더 안정적.

3. **위젯 이벤트 주입**: `MIWidgetSurface`는 충돌 방지 + 터치 이벤트 브로드캐스트까지 구현. 실제 UMG 위젯에 대한 Slate input injection (`FSlateApplication::ProcessMouseButtonDownEvent` 등)은 프로젝트 레벨에서 추가 구현 필요. 비공식 API이므로 엔진 업데이트 시 변경될 수 있음.

4. **Haptic 미지원 글러브**: 현재 보유 글러브는 haptic 미지원. `MIHapticFeedbackManager`는 `DoesSkeletonHaveHaptics()` 확인 후 자동 비활성화되므로 코드 변경 불필요.

5. **Collision profile**: Physics finger body는 `PhysicsActor_ProfileName`을 사용. 프로젝트에 따라 커스텀 collision channel/profile 설정이 필요할 수 있음.

6. **Grab 안정성**: 작은 물체나 고속 이동 시 grab constraint가 떨릴 수 있음. `GrabStiffness`, `GrabDamping` 튜닝 필요.

7. **성능**: 양손 = 12개 physics body (10 finger + 2 palm). 인터랙터블 수가 많으면 overlap query 비용 증가. `MISurfaceInteractable`은 매 틱 `GetAllActorsOfClass`를 호출하는데, 프로덕션에서는 캐싱 또는 proximity 기반으로 최적화 필요.

---

## 다음 단계

1. **UE5 에디터에서 컴파일 확인** — 프로젝트 열고 빌드.
2. **Bone 이름 검증** — Manus 데모 레벨에서 실제 skeleton bone 이름 확인 후 `SetDefaultBoneNames()` 수정.
3. **Collision profile 설정** — 프로젝트 설정에서 ManusInteraction용 커스텀 collision channel 생성.
4. **Physics sub-stepping 활성화** — Project Settings → Physics → Enable Substepping.
5. **프로토타입 레벨** — 벽 + 레버 + 버튼 배치 → Physics Hand 테스트.
6. **`MISurfaceInteractable` 최적화** — `GetAllActorsOfClass` 대신 proximity 기반 쿼리로 교체.
7. **Widget input injection** — 필요 시 `FSlateApplication` 기반 synthetic event 구현 추가.
