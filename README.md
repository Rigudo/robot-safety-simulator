# Robot Safety Simulator 개발 가이드

이 저장소는 Qt Widgets 기반 로봇 안전 상태 판단 시뮬레이터입니다. 현재 코드는 `RiskPage` 중심으로 구성되어 있으며, 이후 `ImagePage`, `LogicPage`, `RiskPage`를 하나의 프로그램으로 통합할 수 있도록 기준 구조를 잡아둔 상태입니다.

---

## 1. 실행 파일

Qt Creator에서 아래 파일을 열면 됩니다.

```text
robotSafety/RobotRiskSimulator.pro
```

현재 실행 흐름은 다음과 같습니다.

```text
RobotRiskSimulator.pro
→ main.cpp
→ RiskPage 생성
→ SafetyModel로 상태 계산
→ SimulationView와 결과 패널에 표시
```

---

## 2. 핵심 파일 구조

현재 실제 코드 작성과 통합에 필요한 핵심 파일은 아래 6개입니다.

```text
robotSafety/
├─ RobotRiskSimulator.pro
├─ main.cpp
├─ SafetyModel.h
├─ SafetyModel.cpp
├─ riskpage.h
└─ riskpage.cpp
```

| 파일 | 역할 | 수정하는 경우 |
|---|---|---|
| `RobotRiskSimulator.pro` | Qt 프로젝트 설정 파일 | 새 `.cpp`, `.h` 파일을 추가할 때 |
| `main.cpp` | 프로그램 시작점 | 메인 화면 구조를 바꿀 때 |
| `SafetyModel.h` | 입력값/결과값 구조 정의 | 판단에 필요한 변수를 추가할 때 |
| `SafetyModel.cpp` | MOVE/WARNING/DANGER/STOP 판단 로직 | 안전 판단 기준을 바꿀 때 |
| `riskpage.h` | RiskPage 화면 클래스 선언 | 버튼, 슬라이더, 함수, 멤버 변수를 추가할 때 |
| `riskpage.cpp` | UI 구성, 시뮬레이션, 로그 구현 | 화면 배치나 동작을 바꿀 때 |

---

## 3. 필요 없는 파일과 폴더

아래 폴더들은 최종 제출용 코드에는 필요 없습니다.

```text
robotSafety/build/
robotSafety/build_check/
robotSafety/build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/
```

이 폴더들은 Qt Creator가 컴파일하면서 자동으로 만든 빌드 결과물입니다. 소스코드가 아니라 실행 파일, 임시 파일, 자동 생성 파일에 가깝습니다.

앞으로 이런 파일이 다시 올라가지 않도록 `.gitignore`를 추가했습니다.

```text
.gitignore
```

이미 GitHub에 올라간 빌드 폴더가 남아 있으면 최종 정리 단계에서 GitHub 웹에서 삭제하거나 로컬에서 아래 명령으로 제거하면 됩니다.

```bash
git rm -r robotSafety/build
git rm -r robotSafety/build_check
git commit -m "Remove build output folders"
git push
```

---

## 4. 전체 구조를 한 파일처럼 이해하는 방법

이 프로젝트는 파일이 여러 개로 나뉘어 있지만 실제 흐름은 하나입니다.

```text
main.cpp
→ RiskPage 화면 생성
→ 사용자가 슬라이더 조절
→ readInput()이 입력값 읽음
→ SafetyModel::evaluate()가 상태 계산
→ updateResultPanel()이 오른쪽 결과 표시
→ SimulationView::paintEvent()가 중앙 그림 표시
→ appendLog()가 로그 기록
```

핵심 흐름은 다음 한 줄입니다.

```text
입력값 읽기 → 안전 판단 → 화면 출력 → 로그 저장
```

---

## 5. 파일별 상세 가이드

## 5.1 `main.cpp`

역할은 프로그램 시작입니다.

현재 구조는 다음과 같습니다.

```text
QApplication 생성
RiskPage 생성
창 제목 설정
창 크기 고정
page.show()
app.exec()
```

보통 이 파일은 많이 수정할 필요가 없습니다.

수정이 필요한 경우는 `RiskPage` 하나만 띄우는 구조에서 `ImagePage / LogicPage / RiskPage`를 모두 포함하는 `MainWindow` 구조로 바꿀 때입니다.

예상 최종 구조는 다음과 같습니다.

```text
main.cpp
→ MainWindow 실행
→ MainWindow 안에 ImagePage / LogicPage / RiskPage 탭 배치
```

현재 `main.cpp`의 핵심 부분은 다음 형태입니다.

```cpp
RiskPage page;
page.show();
```

나중에 전체 통합 구조로 바꿀 때는 다음처럼 바꾸면 됩니다.

```cpp
MainWindow window;
window.show();
```

---

## 5.2 `SafetyModel.h`

이 파일은 안전 판단에 필요한 데이터 형식을 정의합니다.

현재 입력 구조체 역할은 다음과 같습니다.

```text
SafetyInput
= 사람 거리, 장애물 거리, 문 열림, 로봇 속도, 감속도, 비상 신호, 경고선, 정지선 저장
```

현재 결과 구조체 역할은 다음과 같습니다.

```text
SafetyResult
= 상태, 이유, 위험도, 제동거리, 가장 가까운 거리, 안전 여유거리, 권장 속도 저장
```

새 센서를 추가하려면 이 파일부터 수정합니다.

예를 들어 온도 센서를 추가한다면 `SafetyInput` 안에 다음 변수를 추가합니다.

```cpp
double temperature = 25.0;
```

그다음 `SafetyModel.cpp`에서 이 값을 실제 판단에 반영해야 합니다.

---

## 5.3 `SafetyModel.cpp`

이 파일은 실제 판단 로직입니다.

현재 판단 순서는 다음과 같습니다.

```text
1. 제동거리 계산
2. 가장 가까운 물체 거리 계산
3. 안전 여유거리 계산
4. 사람 위험도 계산
5. 장애물 위험도 계산
6. 문 열림 위험도 계산
7. 제동 위험도 계산
8. 비상 신호 위험도 계산
9. 최종 위험도 계산
10. STOP / DANGER / WARNING / MOVE 결정
```

상태 판단 기준을 바꾸려면 이 파일을 수정하면 됩니다.

예를 들어 문 열림 70% 이상에서 STOP인 기준을 80%로 바꾸려면 아래 조건을 찾습니다.

```cpp
input.doorOpenRate >= 70.0
```

그리고 다음처럼 바꿉니다.

```cpp
input.doorOpenRate >= 80.0
```

제동거리 공식은 다음 위치에 있습니다.

```cpp
result.brakeDistance = (input.robotSpeed * input.robotSpeed) / qMax(0.1, 2.0 * input.deceleration);
```

의미는 다음과 같습니다.

```text
제동거리 = 속도^2 / (2 × 감속도)
```

---

## 5.4 `riskpage.h`

이 파일은 `RiskPage`에서 사용할 함수와 변수 목록입니다.

새 버튼, 새 슬라이더, 새 라벨을 추가하려면 먼저 여기에 선언해야 합니다.

예를 들어 온도 슬라이더를 추가하려면 다음 멤버 변수를 추가합니다.

```cpp
QSlider *temperatureSlider = nullptr;
QLabel *temperatureValue = nullptr;
```

그다음 `riskpage.cpp`에서 실제 생성과 배치를 합니다.

---

## 5.5 `riskpage.cpp`

이 파일은 화면과 동작의 핵심입니다.

| 함수 | 역할 |
|---|---|
| `RiskPage::RiskPage()` | 화면 초기화, 타이머 연결, 초기값 세팅 |
| `styleApp()` | 다크/라이트 모드 스타일 설정 |
| `toggleTheme()` | 테마 전환 |
| `buildUi()` | 전체 화면 배치 생성 |
| `buildControlPanel()` | 왼쪽 입력 패널 생성 |
| `buildResultPanel()` | 오른쪽 결과 패널 생성 |
| `buildOutputPanel()` | 아래 로그/요약 패널 생성 |
| `readInput()` | 슬라이더 값을 `SafetyInput`으로 변환 |
| `writeInput()` | `SafetyInput` 값을 슬라이더에 반영 |
| `updateSliderLabels()` | 슬라이더 옆 숫자 표시 갱신 |
| `evaluateAndRender()` | 판단 실행 후 전체 화면 갱신 |
| `updateResultPanel()` | 오른쪽 상태 결과 갱신 |
| `appendLog()` | 로그 테이블에 한 줄 추가 |
| `stepSimulation()` | 시뮬레이션 한 단계 진행 |
| `toggleSimulation()` | 자동 실행 시작/정지 |
| `resetSimulation()` | 전체 초기화 |
| `exportLogCsv()` | 로그를 CSV 파일로 저장 |

---

## 6. 기능을 추가할 때 수정 위치

## 6.1 입력값을 추가할 때

예: 온도, 배터리, 조도, 충돌 센서 등

수정 순서는 다음과 같습니다.

```text
1. SafetyModel.h
   → SafetyInput에 변수 추가

2. riskpage.h
   → QSlider 또는 QCheckBox 멤버 추가
   → QLabel 값 표시 멤버 추가

3. riskpage.cpp / buildControlPanel()
   → 실제 입력 위젯 생성
   → 화면에 배치

4. riskpage.cpp / readInput()
   → UI 값을 SafetyInput에 넣기

5. riskpage.cpp / writeInput()
   → SafetyInput 값을 UI에 반영

6. riskpage.cpp / updateSliderLabels()
   → 슬라이더 옆 값 표시

7. SafetyModel.cpp
   → 실제 판단 로직에 반영

8. riskpage.cpp / updateResultPanel()
   → 결과 패널에 표시
```

---

## 6.2 새로운 상태를 추가할 때

예: `PAUSE`, `EMERGENCY`, `SLOW`, `BLOCKED`

수정 순서는 다음과 같습니다.

```text
1. SafetyModel.cpp
   → 상태 판단 조건 추가

2. riskpage.cpp / stateColor()
   → 새 상태의 색상 추가

3. riskpage.cpp / updateResultPanel()
   → 새 상태의 설명 규칙 추가

4. README.md
   → 상태 설명 추가
```

현재 상태는 다음 4개입니다.

```text
MOVE
WARNING
DANGER
STOP
```

---

## 6.3 중앙 다이어그램을 수정할 때

중앙 그림은 `SimulationView::paintEvent()`에서 그립니다.

위치:

```text
riskpage.cpp
→ SimulationView::paintEvent()
```

여기 안에서 다음 요소를 그립니다.

```text
배경
도로/기준선
STOP 영역
WARNING 영역
브레이크 거리
로봇
사람
장애물
문 열림 상태
위험도 그래프
```

중앙 그림만 수정하고 싶으면 `SafetyModel.cpp`는 건드리지 않아도 됩니다.

로봇 모양만 바꾸고 싶으면 `paintEvent()` 안의 로봇을 그리는 부분만 수정하면 됩니다.

---

## 6.4 왼쪽 입력 패널을 수정할 때

위치:

```text
riskpage.cpp
→ buildControlPanel()
```

여기에서 슬라이더, 체크박스, RUN/Step/Export 버튼을 만듭니다.

입력 항목 이름을 바꾸거나 순서를 바꾸려면 이 함수 안의 `names`, `sliders`, `labels` 부분을 수정하면 됩니다.

---

## 6.5 오른쪽 결과 패널을 수정할 때

위치:

```text
riskpage.cpp
→ buildResultPanel()
→ updateResultPanel()
```

`buildResultPanel()`은 라벨을 처음 만드는 곳입니다.

`updateResultPanel()`은 계산 결과가 바뀔 때마다 라벨 내용을 바꾸는 곳입니다.

즉:

```text
라벨 추가 = buildResultPanel()
라벨 내용 갱신 = updateResultPanel()
```

---

## 6.6 로그 기능을 수정할 때

위치:

```text
riskpage.cpp
→ buildOutputPanel()
→ appendLog()
→ exportLogCsv()
```

`buildOutputPanel()`은 로그 테이블의 열 이름을 정합니다.

`appendLog()`는 실제 로그 한 줄을 추가합니다.

`exportLogCsv()`는 로그를 CSV 파일로 저장합니다.

로그에 새 항목을 추가하려면 세 함수를 같이 수정해야 합니다.

---

## 7. 여러 화면을 하나로 통합하는 구조

최종적으로 화면이 여러 개가 된다면 아래 구조를 추천합니다.

```text
robotSafety/
├─ main.cpp
├─ MainWindow.h
├─ MainWindow.cpp
├─ ImagePage.h
├─ ImagePage.cpp
├─ LogicPage.h
├─ LogicPage.cpp
├─ RiskPage.h
├─ RiskPage.cpp
├─ SafetyModel.h
├─ SafetyModel.cpp
└─ RobotRiskSimulator.pro
```

현재 코드는 `RiskPage`만 독립 실행되는 구조입니다.

최종 통합 때는 다음 흐름으로 바꾸면 좋습니다.

```text
main.cpp
→ RiskPage 직접 실행 X
→ MainWindow 실행 O
→ MainWindow 안에 ImagePage / LogicPage / RiskPage 배치
```

현재 `main.cpp`의 핵심 부분:

```cpp
RiskPage page;
page.show();
```

을 나중에 다음처럼 바꿉니다.

```cpp
MainWindow window;
window.show();
```

---

## 8. `.pro` 파일에 새 파일 추가하는 법

새 `.cpp` 파일을 만들면 `RobotRiskSimulator.pro`의 `SOURCES`에 추가해야 합니다.

예:

```text
SOURCES += \
    main.cpp \
    riskpage.cpp \
    SafetyModel.cpp \
    MainWindow.cpp \
    ImagePage.cpp \
    LogicPage.cpp
```

새 `.h` 파일을 만들면 `HEADERS`에 추가해야 합니다.

예:

```text
HEADERS += \
    riskpage.h \
    SafetyModel.h \
    MainWindow.h \
    ImagePage.h \
    LogicPage.h
```

이걸 안 하면 Qt Creator에서 파일을 못 찾거나 빌드가 꼬일 수 있습니다.

---

## 9. 통합 우선순위

현재 코드 기준으로 아래 순서가 좋습니다.

```text
1. Qt Creator에서 현재 코드 실행 확인
2. 추가할 화면의 파일명 정리
3. ImagePage / LogicPage / RiskPage로 역할 분리
4. MainWindow 만들기
5. main.cpp에서 MainWindow 실행으로 변경
6. RobotRiskSimulator.pro에 새 파일 추가
7. 전체 빌드 확인
8. README와 보고서용 설명 정리
```

---

## 10. 주의할 점

- `SafetyModel.cpp`는 판단 로직 파일입니다. 화면 디자인만 바꾸고 싶으면 건드리지 않는 것이 좋습니다.
- `riskpage.cpp`는 화면과 이벤트가 많아서 충돌이 가장 잘 나는 파일입니다. 가능하면 한 번에 여러 부분을 동시에 수정하지 않는 것이 좋습니다.
- 새 기능은 가능하면 새 파일로 분리하는 것이 좋습니다.
- 최종 제출 전에는 build 폴더를 삭제하고, 소스 파일 중심으로 정리해야 합니다.
- `.gitignore`는 이미 추가되어 있으므로 이후 빌드 결과물이 다시 올라갈 가능성은 줄었습니다.

---

## 11. 현재 권장 구성

현재 코드 기준 권장 구성은 다음과 같습니다.

```text
ImagePage: 이미지 처리 화면
LogicPage: 체크박스 기반 논리 판단 화면
RiskPage: 거리/속도/제동거리 기반 위험도 화면
SafetyModel: 안전 상태 판단 로직
MainWindow: 전체 화면 통합
```

현재 저장소는 `RiskPage` 중심 코드가 먼저 올라간 상태이므로, 다른 화면은 `RiskPage`를 직접 수정하기보다 새 Page 클래스로 추가하는 방향이 안전합니다.
