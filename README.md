## OS 구조와 원리: OS 개발 30일 프로젝트

### 프로젝트 개요

> 본 프로젝트는 카와이 히데미(川合秀実)의 저서 **「OS 구조와 원리」** 를 기반으로 시스템 프로그래밍 및 운영체제 내부 동작 원리에 대한 이해를 목표로 진행한 OS 개발 실습 프로젝트입니다.
>
> 부산대학교 정보컴퓨터공학부 **프로그래밍언어 연구실** 우균 교수님과 김영훈 연구원님의 지도 하에 진행되었으며, Bare-metal 환경에서 실제로 부팅 가능한 간단한 운영체제를 직접 구현하였습니다.

### 구현 기능

- **Bootloader (IPL)** 
    - FAT12 포맷의 플로피 디스크 이미지에서 커널을 메모리로 로드
    - BIOS 인터럽트를 이용한 디스크 섹터 읽기 구현
- **32-bit Mode Switching**
    - 16-bit Real Mode에서 32-bit Protected Mode로 전환
    - GDT(Global Descriptor Table) 구성 및 세그먼트 레지스터 설정

### 기술 스택

> **언어**

- C Language
    - OS 커널 로직 구현
    - 표준 라이브러리 없는 환경에서 저수준 시스템 프로그래밍

- Assembly (NASK)
    - 부트로더 구현
    - CPU 모드 전환, 레지스터 직접 제어

> **빌드 및 개발 환경**

- GCC
- WSL2 (Ubuntu)
- Git

> **하드웨어 추상화 및 에뮬레이션**

- Qemu
- VMware

### 메모리 레이아웃

### 트러블 슈팅

### 프로젝트 구조

```
.
├── Makefile
├── img                      // 디스크 이미지 파일
│   ├── haribote.sys
│   └── haribote.img
├── out                      // 빌드 과정에서 발생하는 파일
├── src                      // 소스 코드
│   ├── asm
│   │   └── naskfunc.nas
│   ├── boot
│   │   ├── asmhead.nas
│   │   └── ipl10.nas
│   ├── include
│   └── kernel
│       └── bootpack.c
├── tool                     // 교재 빌드 툴셋(리눅스 버전)
└── vm                       // vmware 가상머신
```
