[개발환경]
Windows, 언리얼엔진 5.3, VS2022

[언어] 
C++, 블루프린트

2인으로 플레이하는 3인칭 슈팅게임.
총 30웨이브의 몰려오는 AI Bot들을 상대합니다.
플레이어를 추적하고, 일정 거리 접근 시 스스로 자폭하는 TrackerBot.
BehaviorTree와 EQS를 사용하여 플레이어를 추적, 공격하는 Advanced AI가 적으로 있습니다.
Advanced AI는 샷건, 스나이퍼 라이플, 로켓 런처, 그레네이드 런처 등의 무기로 플레이어를 공격합니다.

Host : 스팀 로그인 후 호스트를 클릭하면 방 생성을 합니다..
Join : 스팀 로그인 후 조인 버튼을 누르면 방 입장을 합니다.

게임 실행 후 방 생성 혹은 참가에 성공할 시 Lobby 맵으로 이동되며,
로비에서는 라이플, 스나이퍼 라이플, 샷건, SMG, 로켓 런처, 그레네이드 런처 등의 무기를 사용해
허수아비를 공격해 대미지나 5초 누적 대미지를 확인해 볼 수 있습니다.

플레이어 수가 2명이 될 시 잠시 후 게임 맵으로 이동합니다.

[키 설명]
W,A,S,D : 이동
E : 총 줍기, 무기 스왑
Shift : 앉기
RMB : 에임
LMB : 발사
R : 재장전
ESC, Q : 나가기

[게임실행]
CoopShooter > CoopShooter.exe
메인폴더 > CoopShooter.exe - 바로가기

[플레이 영상]
https://youtu.be/W8zzBklaunE

[참고]
Unreal Engine 4 Mastery: Create Multiplayer Games with C++
Unreal Engine 5 C++ Multiplayer Shooter
