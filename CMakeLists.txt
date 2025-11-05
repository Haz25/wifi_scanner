cmake_minimum_required(VERSION 3.8)
project(wifi_scanner)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rosidl_typesupport_cpp REQUIRED)  # <- 추가(안전)

# 1) 메시지 생성
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/WifiScan.msg"
  "msg/WifiTop.msg"
  DEPENDENCIES builtin_interfaces
)

# 2) 실행파일
add_executable(wifi_scanner_node src/wifi_scanner_node.cpp)
ament_target_dependencies(wifi_scanner_node rclcpp)

# 인터페이스 헤더 경로 포함 + 빌드 순서 보장
target_include_directories(wifi_scanner_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
)
add_dependencies(wifi_scanner_node ${PROJECT_NAME}__rosidl_generator_cpp)

# 3) (핵심) typesupport 라이브러리와 링크
#   rosidl_get_typesupport_target을 써서 이 패키지의 C++ 타입서포트 타겟 이름을 가져옵니다.
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(wifi_scanner_node ${cpp_typesupport_target})

install(TARGETS wifi_scanner_node DESTINATION lib/${PROJECT_NAME})

ament_export_dependencies(rosidl_default_runtime)
ament_package()