# OS

# 06-03 
소스 파일과 헤더 파일 생성 및 실행
스레드 증가에 따른 처리율 속도 비교

# 06-09
1. queue.cpp
실제 힙 메모리에 값 저장하도록 하여 깊은 복사가 가능하게 바꿈
value 메모리 해제 추가
클라이언트 스레드 갯수 변경
item.value_size 크기만큼 유동적으로 복사
value_size 필드 포함, item.value_size로 메모리 관리
deep_copy(item.value, item.value_size)로 변경

2. main.cpp
깊은 복사구현이 가능하도록 변경
메모리 안전성 높임
Item에 value_size 필드 추가 및 설정
value_size를 명시하여 큐 내부에서 안전하게 deep_copy 가능
Reply reply = { false, {0, nullptr, 0} }; → 구조체 확장에 맞게 수정

3. qtype.h
key, value + value_size 추가
   
