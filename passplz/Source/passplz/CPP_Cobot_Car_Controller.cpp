// Fill out your copyright notice in the Description page of Project Settings.

//기존 컨트롤러에서 했던거 패킷 처리 필요

#define _CRT_SECURE_NO_WARNINGS

#include "CPP_Cobot_Car_Controller.h"
#include "CPP_Cobot_Car.h"
//#include "Engine/World.h"
#include "CPP_Stage3Cobot.h"
#include "CPP_Cannon.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "../../../server/server/protocol.h"

ACPP_Cobot_Car_Controller::ACPP_Cobot_Car_Controller()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>DEFAULT_CONTEXT
	(TEXT("/Game/K_Test/input/stage3IMCTest.stage3IMCTest"));
	if (DEFAULT_CONTEXT.Succeeded())
		DefaultContext = DEFAULT_CONTEXT.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>IA_MOVE
	(TEXT("/Game/K_Test/input/Move.Move"));
	if (IA_MOVE.Succeeded())
		Move = IA_MOVE.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>IA_MOUSEWHEEL
	(TEXT("/Game/K_Test/input/MouseWheel.MouseWheel"));
	if (IA_MOUSEWHEEL.Succeeded())
		MouseWheel = IA_MOUSEWHEEL.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>IA_MOUSELEFT
	(TEXT("/Game/K_Test/input/MouseLeft.MouseLeft"));
	if (IA_MOUSELEFT.Succeeded())
		MouseLeft = IA_MOUSELEFT.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>IA_ROTATE
	(TEXT("/Game/K_Test/input/rotate.rotate"));
	if (IA_ROTATE.Succeeded())
		Rotate = IA_ROTATE.Object;
}

ACPP_Cobot_Car_Controller::~ACPP_Cobot_Car_Controller()
{
}

void ACPP_Cobot_Car_Controller::BeginPlay()
{
	Super::BeginPlay();

	instance = Cast<UCPP_CobotGameInstance>(GetWorld()->GetGameInstance());
	sock = instance->GetSocketMgr()->GetSocket();

	cs_stage3_enter_packet pack;
	pack.size = sizeof(pack);
	pack.type = static_cast<char>(packet_type::cs_stage3_enter);

	int ret = send(*sock, reinterpret_cast<char*>(&pack), sizeof(pack), 0);

	UE_LOG(LogTemp, Warning, TEXT("car controller enter!"));

	if (UEnhancedInputLocalPlayerSubsystem* SubSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		SubSystem->AddMappingContext(DefaultContext, 0);

	player = Cast<ACPP_Cobot_Car>(GetPawn());
	if (!player)
		return;

	cannon = UGameplayStatics::GetActorOfClass(GetWorld(), ACPP_Cannon::StaticClass());
	if(cannon)
		UE_LOG(LogTemp, Warning, TEXT("cannon OK"));





}

void ACPP_Cobot_Car_Controller::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ACPP_Cobot_Car_Controller::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	UE_LOG(LogTemp, Warning, TEXT("OnPossess"));
}

void ACPP_Cobot_Car_Controller::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		//자동차 움직이기
		EnhancedInputComponent->BindAction(Move, ETriggerEvent::Triggered, this, &ACPP_Cobot_Car_Controller::CarInput);
		//조준점 조정
		EnhancedInputComponent->BindAction(MouseWheel, ETriggerEvent::Triggered, this, &ACPP_Cobot_Car_Controller::CannonInput);
		//발사
		EnhancedInputComponent->BindAction(MouseLeft, ETriggerEvent::Triggered, this, &ACPP_Cobot_Car_Controller::FireCannonInput);
		//시점 변화
		EnhancedInputComponent->BindAction(Rotate, ETriggerEvent::Triggered, this, &ACPP_Cobot_Car_Controller::RotateInput);
	}
}

void ACPP_Cobot_Car_Controller::RecvPacket()
{
	char recv_buff[BUF_SIZE];

	int recv_ret = recv(*sock, reinterpret_cast<char*>(&recv_buff), BUF_SIZE, 0);
	if (recv_ret <= 0)
	{
		//GetLastError();
		//std::cout << "recv() fail!" << std::endl;
		return;
	}

	int ring_ret = ring_buff.enqueue(recv_buff, recv_ret);
	if (static_cast<int>(error::full_buffer) == ring_ret) {
		//std::cout << "err: ring buffer is full\n";
		return;
	} else if (static_cast<int>(error::in_data_is_too_big) == ring_ret) {
		//std::cout << "err: in data is too big\n";
		return;
	}

	int buffer_start = 0;
	while (ring_buff.remain_data() > 0)
	{
		char pack_size = recv_buff[buffer_start];
		if (pack_size <= ring_buff.remain_data()) {
			char dequeue_data[BUFFER_SIZE];

			ring_ret = ring_buff.dequeue(reinterpret_cast<char*>(&dequeue_data), pack_size);
			if (static_cast<int>(error::no_data_in_buffer) == ring_ret)
				break;
			else if (static_cast<int>(error::out_data_is_too_big) == ring_ret)
				break;

			ProcessPacket(dequeue_data);

			buffer_start += pack_size;
		} else break;
	}
}

void ACPP_Cobot_Car_Controller::ProcessPacket(char* packet)
{
	switch (packet[1])
	{
	case static_cast<int>(packet_type::sc_stage3_enter):
	{
		sc_stage3_enter_packet* pack = reinterpret_cast<sc_stage3_enter_packet*>(packet);
		player_number = pack->player_number;
	} break;
	case static_cast<int>(packet_type::sc_car_direction):
	{
		//UE_LOG(LogTemp, Warning, TEXT("recv sc_car_direction"));

		sc_car_direction_packet* pack = reinterpret_cast<sc_car_direction_packet*>(packet);

		UE_LOG(LogTemp, Warning, TEXT("direction: %lf"), pack->direction);

		if (0.0 == pack->direction) {

			//CarForward(서버에서 받는 값이 필요);
			player->ChangAim(true, true);
			;
		}
		else {
			if (pack->direction > 0.0) {

				player->ChangAim(false, true);
			}
		
			else
				player->ChangAim(true, false);

			CarRotation(pack->direction);
		}

	} break;
	case static_cast<int>(packet_type::sc_cannon_yaw):
	{
		sc_cannon_yaw_packet* pack = reinterpret_cast<sc_cannon_yaw_packet*>(packet);

		Cast<ACPP_Cannon>(cannon)->SetBombDropLocation(1, pack->yaw);
	} break;
	case static_cast<int>(packet_type::sc_cannon_pitch):
	{
		sc_cannon_pitch_packet* pack = reinterpret_cast<sc_cannon_pitch_packet*>(packet);

		Cast<ACPP_Cannon>(cannon)->SetBombDropLocation(2, pack->pitch);
	} break;
	case static_cast<int>(packet_type::sc_cannon_click):
	{
		sc_cannon_click_packet* pack = reinterpret_cast<sc_cannon_click_packet*>(packet);

		if (player_number == pack->click_id) {
			// 자기가 누른거
			Cast<ACPP_Cannon>(cannon)->FireLava();
		}
		else {
			// 상대방이 누른거
			Cast<ACPP_Cannon>(cannon)->FireLava();
		}
	} break;
	case static_cast<int>(packet_type::sc_cannon_fire):
	{
		// 대포 발사하는 함수 호출
		Cast<ACPP_Cannon>(cannon)->FireLava();
	} break;
	}
}

void ACPP_Cobot_Car_Controller::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RecvPacket();
}

void ACPP_Cobot_Car_Controller::CarInput(const FInputActionValue& Value)
{
	//서버 : 여기서 키를 누르거나 떼면 여길 들어오는데 이때 서버에 패킷 보내야함
	//서버에는 각 클라 키에 대한 bool값을 가진다.
	//서버에서 가속과 관련된 계산이 필요합니다. -> 점점 커지는 값 그리고 뗐을때는 점점 작아지는 값 필요 그 값을 넘겨주세요
	UE_LOG(LogTemp, Warning, TEXT("%f"), Value.Get<float>());

	cs_car_direction_packet pack;
	pack.size = sizeof(pack);
	pack.type = static_cast<char>(packet_type::cs_car_direction);

	if (0.0 != Value.Get<float>())
		pack.direction = true;
	else
		pack.direction = false;

	int ret = send(*sock, reinterpret_cast<char*>(&pack), sizeof(pack), 0);

	if (mode == 0) {
		UE_LOG(LogTemp, Warning, TEXT("CarInput %f"), Value.Get<float>());
	}
}


void ACPP_Cobot_Car_Controller::CannonInput(const FInputActionValue& Value)
{
	if (mode == 1) {
		UE_LOG(LogTemp, Warning, TEXT("CannonInput %f"), Value.Get<float>());
	}

	cs_cannon_packet pack;
	pack.size = sizeof(pack);
	pack.type = static_cast<char>(packet_type::cs_cannon);
	pack.cannon_value = Value.Get<float>();

	send(*sock, reinterpret_cast<char*>(&pack), sizeof(pack), 0);
}

void ACPP_Cobot_Car_Controller::FireCannonInput(const FInputActionValue& Value)
{
	if (mode == 1) {
		UE_LOG(LogTemp, Warning, TEXT("FireCannonInput %s"), Value.Get<bool>() ? TEXT("true") : TEXT("false"));
		Cast<ACPP_Cannon>(cannon)->FireLava();
	}

	if (1.f == Value.Get<float>()) {
		cs_cannon_click_packet pack;
		pack.size = sizeof(pack);
		pack.type = static_cast<char>(packet_type::cs_cannon_click);

		send(*sock, reinterpret_cast<char*>(&pack), sizeof(pack), 0);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("FireCannonInput"));
	}
}

void ACPP_Cobot_Car_Controller::RotateInput(const FInputActionValue& Value)
{
	//컨트롤러가 아닌 플레이어에 있는 스프링암을 회전해야함
	//컨트롤러 회전은 서버에서 결정한다.
	player->SpringArm->AddRelativeRotation(FRotator(Value.Get<FVector2D>().Y, Value.Get<FVector2D>().X, 0.0f));
}

void ACPP_Cobot_Car_Controller::CarForward(float acceleration)
{
	UE_LOG(LogTemp, Warning, TEXT("CarForward"));
	player->AddActorWorldOffset(player->GetActorForwardVector() * acceleration);
}

void ACPP_Cobot_Car_Controller::CarRotation(float rotationValue)
{
	FRotator control_rotation = GetControlRotation();
	control_rotation.Yaw += rotationValue;
	SetControlRotation(control_rotation);
}

void ACPP_Cobot_Car_Controller::ChangeMode(int Mode)
{
	UE_LOG(LogTemp, Warning, TEXT("ChangeMode"));

	switch (Mode)
	{
		//일반 모드
		//카메라 설정 같은것도 필요함
	case 0:
		mode = 0;
		break;
		//대포 모드
	case 1:
		mode = 1;
		break;

	default:
		break;
	}
}





