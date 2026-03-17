# ManusInteraction Plugin — Blueprint Setup Guide

이 가이드는 ManusInteraction 플러그인의 C++ 컴포넌트들을 Blueprint에서 조합하여 인터랙터블 오브젝트를 만드는 방법을 설명한다.

---

## 1. 사전 준비

### 1-1. 플러그인 활성화

1. `ManusProject.uproject`에 플러그인 추가:
```json
{
  "Plugins": [
    {
      "Name": "ManusInteraction",
      "Enabled": true
    }
  ]
}
```
2. 에디터 재시작 후 Plugins 패널에서 ManusInteraction이 활성화되어 있는지 확인.

### 1-2. Collision 설정 (권장)

Project Settings → Collision:
1. New Object Channel 추가: `ManusPhysicsFinger`
2. New Collision Profile 추가: `PhysicsFinger`
   - Object Type: `ManusPhysicsFinger`
   - Block: WorldStatic, WorldDynamic, PhysicsBody
   - Overlap: Pawn
   - Ignore: (기타 불필요한 채널)

> 이 단계는 선택사항. 기본 `PhysicsActor` 프로필로도 동작하지만, 커스텀 채널을 쓰면 finger끼리의 충돌 등을 세밀하게 제어할 수 있다.

### 1-3. Physics Sub-stepping (권장)

Project Settings → Physics:
- `Enable Substepping` = true
- `Max Substep Delta Time` = 0.008333 (120Hz)
- `Max Substeps` = 4

> VR 환경에서 얇은 충돌면 관통 방지를 위해 권장.

---

## 2. Physics Hand 세팅

### 2-1. Physics Hand Actor 배치

**방법 A: C++ 직접 스폰 (권장)**

GameMode 또는 Pawn의 BeginPlay에서:

```cpp
// 왼손 Physics Hand 스폰
AMIPhysicsHand* LeftPhysHand = GetWorld()->SpawnActor<AMIPhysicsHand>(
    AMIPhysicsHand::StaticClass(), FTransform::Identity);
LeftPhysHand->SourceManusComponent = YourLeftManusComponent;
LeftPhysHand->HandSide = EMIHandSide::Left;
LeftPhysHand->InitializeHand();

// 오른손 Physics Hand 스폰
AMIPhysicsHand* RightPhysHand = GetWorld()->SpawnActor<AMIPhysicsHand>(
    AMIPhysicsHand::StaticClass(), FTransform::Identity);
RightPhysHand->SourceManusComponent = YourRightManusComponent;
RightPhysHand->HandSide = EMIHandSide::Right;
RightPhysHand->InitializeHand();
```

**방법 B: Blueprint 스폰**

1. 레벨 Blueprint 또는 GameMode Blueprint 열기
2. BeginPlay에서:
   - `Spawn Actor from Class` → `MIPhysicsHand`
   - `Set Source Manus Component` → 기존 Manus Pawn의 ManusComponent 참조
   - `Set Hand Side` → Left 또는 Right
   - `Initialize Hand` 호출

### 2-2. Physics Hand 설정 조정

MIPhysicsHand Actor를 선택하고 Details 패널에서:

| 프로퍼티 | 기본값 | 설명 |
|----------|--------|------|
| `Config.MaxLinearSpeed` | 800 cm/s | 손가락 physics body의 최대 이동 속도. 높이면 반응 빨라지나 관통 위험. |
| `Config.MaxAngularSpeed` | 1080 deg/s | 최대 회전 속도 |
| `Config.FingerTipRadius` | 1.0 cm | 손가락 sphere 반지름. 키우면 충돌 감지가 쉽지만 정밀도 하락. |
| `Config.PalmRadius` | 3.0 cm | 손바닥 sphere 반지름 |
| `Config.FingerTipMass` | 0.05 kg | 손가락 질량. 작으면 물체를 밀기 어려움. |
| `Config.DriveMode` | Velocity | Velocity(기본, 권장) 또는 Force |
| `bShowDebugVisualization` | false | true로 하면 sphere와 target 사이 선 표시 |

### 2-3. Bone 이름 커스터마이즈

Manus 표준 hand skeleton과 다른 skeleton을 쓸 경우:

1. MIPhysicsHand의 Details → `Finger Tip Bone Names` 확장
2. 각 finger(Thumb, Index, Middle, Ring, Pinky)에 대해 실제 bone 이름 설정
3. `Palm Bone Name`도 설정

> Manus 기본값: `l_finger_thumb_03_end`, `l_finger_index_03_end` 등 (왼손) / `r_...` (오른손)

---

## 3. 인터랙터블 Blueprint 만들기

### 공통 패턴

모든 인터랙터블 Blueprint는 동일한 패턴을 따른다:

```
1. Actor Blueprint 생성
2. Root: DefaultSceneRoot
3. 자식: StaticMeshComponent (인터랙터블 오브젝트의 메시)
4. 컴포넌트 추가: MI[Type]Interactable (C++ 컴포넌트)
5. Details에서 설정값 조정
6. 이벤트 그래프에서 OnInteraction* 바인딩
```

---

### 3-1. 턴 시그널 레버 (MIHingeInteractable)

**Blueprint 생성:**
1. Content Browser → 우클릭 → Blueprint Class → Actor → `BP_TurnSignalLever`
2. Components 추가:
   - `StaticMeshComponent` → 레버 메시 할당
   - `MIHingeInteractable` 컴포넌트 추가

**MIHingeInteractable 설정:**

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `HingeAxis` | Y | 레버가 위아래로 움직이므로 Y축 회전 |
| `MinAngle` | -30 | 아래로 30도 (우회전 표시등) |
| `MaxAngle` | 30 | 위로 30도 (좌회전 표시등) |
| `bAutoReturn` | false | 턴 시그널은 위치 유지 |
| `SnapPositions` | [0.0, 0.5, 1.0] | 아래(-30°), 중앙(0°), 위(+30°) 3개 위치 스냅 |
| `SnapForce` | 5 | 스냅 간 전환에 필요한 힘 |
| `HingeDamping` | 5 | 적당한 저항감 |
| `HingeMass` | 0.1 | 레버 질량 |

**이벤트 그래프:**
```
[MIHingeInteractable] OnHingeSnapped(SnapIndex, NormalizedValue)
    │
    ├── SnapIndex == 0 → Turn Right Signal ON
    ├── SnapIndex == 1 → Signals OFF
    └── SnapIndex == 2 → Turn Left Signal ON
```

---

### 3-2. 와이퍼 레버 (MIHingeInteractable)

위와 동일한 방식. 설정값만 다르게:

| 프로퍼티 | 값 |
|----------|-----|
| `HingeAxis` | Y |
| `MinAngle` | -30 |
| `MaxAngle` | 30 |
| `SnapPositions` | [0.0, 0.33, 0.66, 1.0] |

4개 스냅: OFF, 간헐, 저속, 고속

---

### 3-3. 도어 핸들 (MIHingeInteractable)

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `HingeAxis` | Z | 수평 회전 |
| `MinAngle` | 0 | 닫힌 상태 |
| `MaxAngle` | 45 | 완전히 열린 상태 |
| `bAutoReturn` | true | 손 떼면 자동 복귀 |
| `ReturnSpeed` | 90 | 빠른 복귀 |
| `RestAngle` | 0 | 닫힌 위치로 복귀 |

**이벤트 그래프:**
```
[MIHingeInteractable] OnInteractionUpdate(Finger, Value)
    │
    └── Value > 0.8 → Trigger Door Open Event
```

---

### 3-4. 선바이저 (MIHingeInteractable)

| 프로퍼티 | 값 |
|----------|-----|
| `HingeAxis` | X |
| `MinAngle` | 0 |
| `MaxAngle` | 160 |
| `bAutoReturn` | false |
| `HingeDamping` | 5 |

---

### 3-5. 글러브박스 뚜껑 (MIHingeInteractable)

| 프로퍼티 | 값 |
|----------|-----|
| `HingeAxis` | X |
| `MinAngle` | 0 |
| `MaxAngle` | 70 |
| `bAutoReturn` | false |
| `HingeDamping` | 2 |

---

### 3-6. 에어컨 버튼 (MIButtonInteractable)

**Blueprint 생성:**
1. `BP_ACButton` Actor Blueprint
2. Components: `StaticMeshComponent` (버튼 캡 메시) + `MIButtonInteractable`

**설정:**

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `PressAxis` | X | 버튼이 눌리는 방향 |
| `PressDepth` | 0.5 | 0.5cm 눌림 깊이 |
| `ReturnForce` | 100 | 스프링 복귀 |
| `ActivationThreshold` | 0.7 | 70% 깊이에서 활성화 |
| `bIsToggle` | true | 에어컨 ON/OFF 토글 |
| `ButtonMass` | 0.02 | 가벼운 버튼 |

**이벤트 그래프:**
```
[MIButtonInteractable] OnButtonToggled(bNewState)
    │
    ├── bNewState == true  → AC ON
    └── bNewState == false → AC OFF
```

---

### 3-7. 시동 버튼 (MIButtonInteractable)

| 프로퍼티 | 값 |
|----------|-----|
| `PressAxis` | X |
| `PressDepth` | 1.0 |
| `ActivationThreshold` | 0.9 |
| `bIsToggle` | true |
| `ReturnForce` | 200 |

---

### 3-8. 볼륨 다이얼 (MIDialInteractable)

**Blueprint 생성:**
1. `BP_VolumeKnob` Actor Blueprint
2. Components: `StaticMeshComponent` (노브 메시) + `MIDialInteractable`

**설정:**

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `RotationAxis` | Z | 수직축 회전 |
| `MinAngle` | 0 |  |
| `MaxAngle` | 270 | 270도 범위 |
| `bInfiniteRotation` | false |  |
| `Steps` | 0 | 연속 (무단계) |
| `DialDamping` | 3 | 적당한 저항 |

**이벤트 그래프:**
```
[MIDialInteractable] OnDialRotated(Angle, NormalizedValue)
    │
    └── Set Audio Volume = NormalizedValue
```

---

### 3-9. 온도 다이얼 (MIDialInteractable)

| 프로퍼티 | 값 |
|----------|-----|
| `RotationAxis` | Z |
| `MinAngle` | 0 |
| `MaxAngle` | 180 |
| `Steps` | 10 |

10단계 온도 조절. `OnDialStepped(StepIndex)` 로 단계별 온도 설정.

---

### 3-10. 선루프 슬라이더 (MISliderInteractable)

**Blueprint 생성:**
1. `BP_SunroofSlider` Actor Blueprint
2. Components: `StaticMeshComponent` (슬라이더 핸들 메시) + `MISliderInteractable`

**설정:**

| 프로퍼티 | 값 |
|----------|-----|
| `SlideAxis` | Y |
| `SlideDistance` | 30 |
| `SliderDamping` | 5 |
| `Steps` | 0 |

**이벤트 그래프:**
```
[MISliderInteractable] OnSliderMoved(NormalizedValue)
    │
    └── Set Sunroof Open Percentage = NormalizedValue * 100
```

---

### 3-11. 인포테인먼트 터치스크린 (MISurfaceInteractable + MIWidgetSurface)

**Blueprint 생성:**
1. `BP_Infotainment` Actor Blueprint
2. Components:
   - `StaticMeshComponent` (스크린 패널 메시)
   - `WidgetComponent` (UMG 위젯 표시용)
   - `MISurfaceInteractable` (터치 감지)
   - `MIWidgetSurface` (위젯 관통 방지)

**MISurfaceInteractable 설정:**

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `SurfaceExtent` | (50, 30) | 50cm x 30cm 스크린 |
| `SurfaceNormalAxis` | X | 스크린 전면 방향 |
| `TouchDetectionDistance` | 2.0 | 2cm 이내 터치 감지 |

**MIWidgetSurface 설정:**

| 프로퍼티 | 값 |
|----------|-----|
| `TargetWidget` | (WidgetComponent 참조) |
| `CollisionThickness` | 0.5 |
| `CollisionOffset` | 0.2 |

**이벤트 그래프:**
```
[MISurfaceInteractable] OnSurfaceTouched(Finger, TouchUV, Pressure)
    │
    └── Convert UV to Widget Action
        ├── UV in (0.0-0.3, 0.0-0.5) → Media Button
        ├── UV in (0.3-0.6, 0.0-0.5) → Navigation Button
        └── UV in (0.6-1.0, 0.0-0.5) → Climate Button
```

---

### 3-12. 스티어링 휠 (MISteeringWheelComponent)

**Blueprint 생성:**
1. `BP_SteeringWheel` Actor Blueprint
2. Components:
   - `StaticMeshComponent` (스티어링 휠 메시, Simulate Physics = **false**)
   - `MISteeringWheelComponent`

**MISteeringWheelComponent 설정:**

| 프로퍼티 | 값 | 이유 |
|----------|-----|------|
| `LeftHand` | (Physics Hand Left 참조) | 왼손 그랩 감지용 |
| `RightHand` | (Physics Hand Right 참조) | 오른손 그랩 감지용 |
| `WheelMesh` | (StaticMeshComponent 참조) | 회전 대상 메시 |
| `WheelRotationAxis` | X | 휠 칼럼(축) 방향에 따라 X 또는 Z |
| `MaxLockAngle` | 450 | 좌우 각 450° (1.25회전) |
| `bAutoReturn` | true | 손 떼면 중앙(0°) 복귀 |
| `ReturnSpeed` | 180 | 초당 180° 복귀 |
| `MinGrabFingers` | 2 | 최소 2개 손가락 접촉 시 그랩 |
| `bRequireThumb` | true | 엄지 접촉 필수 |

> 물리 constraint를 사용하지 않고 직접 회전 방식(`SetRelativeRotation`). 양손 동시 조작 및 핸드오프(오른손→왼손 전환 시 각도 유지)를 자연스럽게 지원한다.

**이벤트 그래프:**
```
[MISteeringWheelComponent] OnWheelRotated(Angle, NormalizedValue)
    │
    └── Set Vehicle Steering Input = NormalizedValue  (-1.0 ~ +1.0)

[MISteeringWheelComponent] OnHandGrabbed(HandSide)
    │
    └── (선택) 그랩 시 시각/사운드 피드백

[MISteeringWheelComponent] OnWheelLocked(bAtPositiveLimit)
    │
    └── (선택) 한계각 도달 시 진동/사운드 피드백
```

---

### 3-13. 기어 레버 (MIGrabbableComponent + MIHingeInteractable)

| 컴포넌트 | 설정 |
|----------|------|
| `MIGrabbableComponent` | bIsGrabbable=true |
| `MIHingeInteractable` | Axis=Y, Range=[-20, 20], SnapPositions=[0, 0.25, 0.5, 0.75, 1.0] |

잡아서 앞뒤로 움직이는 기어 레버. 5개 스냅 (P, R, N, D, L).

---

## 4. 새 인터랙터블 타입 추가하기

기존 6가지 패턴으로 커버되지 않는 인터랙션이 필요한 경우:

### 방법 A: Blueprint에서 기존 컴포넌트 조합

대부분의 경우 Hinge + Slider + Button + Dial + Surface + Grab의 조합으로 해결된다.

예: 2축 조이스틱 = MIHingeInteractable × 2 (X축 + Y축을 중첩)

### 방법 B: C++ 새 컴포넌트 작성

1. `MIInteractableBase`를 상속한 새 클래스 생성
2. `BeginPlay()`에서 PhysicsConstraint 설정
3. `TickComponent()`에서 값 업데이트
4. `FindPhysicsMesh()` 활용하여 소유 Actor의 메시 참조

```cpp
UCLASS(Blueprintable, ClassGroup = (ManusInteraction), meta = (BlueprintSpawnableComponent))
class UMICustomInteractable : public UMIInteractableBase
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        // 커스텀 physics constraint 설정
    }

    virtual void TickComponent(float DeltaTime, ...) override
    {
        Super::TickComponent(DeltaTime, ...);
        // 값 업데이트 + 이벤트 브로드캐스트
    }
};
```

---

## 5. 디버깅 팁

### Debug Visualization
```
MIPhysicsHand → bShowDebugVisualization = true
```
- 녹색 sphere: finger physics body 위치
- 빨간 sphere: 접촉 중인 finger
- 노란 선: physics body → kinematic target (divergence 시각화)

### 로그
```
LogManusInteraction: Log    → 초기화, 설정 완료 메시지
LogManusInteraction: Warning → 설정 오류 (mesh 없음, component 없음 등)
```

Output Log 필터에서 `LogManusInteraction` 검색.

### 흔한 문제

| 증상 | 원인 | 해결 |
|------|------|------|
| 손가락이 표면을 관통 | MaxLinearSpeed 너무 높음, 또는 충돌면이 너무 얇음 | Speed 줄이기, collision box 두께 늘리기, sub-stepping 활성화 |
| 레버가 안 움직임 | Finger mass 너무 작음 | FingerTipMass 높이기 (0.05 → 0.1) |
| 버튼이 안 눌림 | PressDepth 너무 큼, 또는 ReturnForce 너무 큼 | 값 줄이기 |
| Grab이 안 됨 | MIGrabbableComponent 미추가, 또는 mesh에 physics 미활성화 | 확인 후 추가 |
| Haptic이 안 됨 | 글러브가 haptic 미지원 | 정상. haptic 글러브 연결 시 자동 작동 |
| 손가락이 아예 안 보임 | SourceManusComponent 미설정 | InitializeHand() 전에 설정 확인 |
| Bone 이름 불일치 | Skeleton이 기본과 다름 | FingerTipBoneNames TMap 수동 설정 |
