cmake_minimum_required (VERSION 3.16)
project (W-Judger)

find_library (config++ config++ REQUIRED)
find_library (seccomp seccomp REQUIRED)
find_package (glog 0.7.1 REQUIRED)
#find_package (Protobuf REQUIRED)
find_package (gRPC CONFIG REQUIRED)

add_executable (wjudger
    src/common.h
    src/jail.h
    src/judger.cc
    src/judger.h
    src/judge-task.cc
    src/judge-task.h
    src/main.cc
    src/sandbox.cc
    src/sandbox.h
    src/syscall_whitelist.h
    src/wjudger.grpc.pb.cc
    src/wjudger.grpc.pb.h
    src/wjudger.pb.cc
    src/wjudger.pb.h
    src/w-server.cc
    src/w-server.h
)
target_link_libraries (wjudger glog::glog)
#target_link_libraries (wjudger ${Protobuf_LIBRARIES})
target_link_libraries (wjudger gRPC::grpc++)
target_link_libraries (wjudger config++)

add_executable (wjudger-launcher
    src/wlauncher.cc
    src/jail.h
    src/syscall_whitelist.h
)
target_link_libraries (wjudger-launcher seccomp)

add_library (libwjudger_seccomp_loader STATIC
  src/libwjudger_seccomp_loader.c
  src/syscall_whitelist.h
)
