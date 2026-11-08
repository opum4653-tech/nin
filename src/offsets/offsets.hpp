#pragma once

#include <cstdint>

// CENTRALIZED OFFSETS MODULE — LONG-TERM MAINTAINABILITY FOUNDATION
// When Free Fire updates (e.g. OB56, OB57), ONLY UPDATE THIS FILE!

namespace Offsets {

// Target Game Build Metadata
    constexpr const char* GameVersion   = "OB55 / 1.132.1";
    constexpr const char* TargetPackage = "com.dts.freefireth";

// Base Chain (GameFacade → BaseGame → Match)
    namespace Base {
        constexpr uintptr_t GameFacade           = 0xA72E4E0;
        constexpr uintptr_t GameFacade_P2        = 0xB8;
        constexpr uintptr_t BaseGame_Match       = 0x90;
        constexpr uintptr_t BaseGame_Timer       = 0x20;
        constexpr uintptr_t Timer_Delta          = 0x2C;
        constexpr uintptr_t MatchIsRunning       = 0xCC;
        constexpr uintptr_t Match_PlayerDict     = 0xC0;
        constexpr uintptr_t Match_LocalPlayer    = 0xD8;
        constexpr uintptr_t Match_VehicleDict    = 0x188;
        constexpr uintptr_t Match_MetaManager    = 0xB8;
    }

// Il2Cpp Mono Dictionary Traversal
    namespace Dict {
        constexpr uintptr_t Entries              = 0x18;
        constexpr uintptr_t Count                = 0x20;
        constexpr uintptr_t ArrayHeader          = 0x20;
        constexpr uintptr_t EntryStride          = 0x18;
        constexpr uintptr_t EntryValue           = 0x10;
    }

// Player Entity Fields
    namespace Player {
        constexpr uintptr_t Rotation             = 0x614;   // Vector3 (Pitch, Yaw, Roll)
        constexpr uintptr_t HeadTF               = 0x6A0;   // ITransformNode
        constexpr uintptr_t FootTF               = 0x6E8;   // ITransformNode
        constexpr uintptr_t Camera               = 0x690;   // FollowCamera
        constexpr uintptr_t IsFiring             = 0x848;   // Boolean
        constexpr uintptr_t HP                   = 0x70;    // HP Pool Pointer
        constexpr uintptr_t Armor                = 0x74;    // Int32
        constexpr uintptr_t IsDead               = 0x7C;    // Boolean
        constexpr uintptr_t Avatar               = 0x770;   // AvatarManager
        constexpr uintptr_t HeadCollider         = 0x738;   // Collider
        constexpr uintptr_t IsBot                = 0x4A0;   // Boolean
        constexpr uintptr_t Name                 = 0x490;   // MonoString*
        constexpr uintptr_t Speed                = 0x560;   // Single (Default 1.0f)
        constexpr uintptr_t AimInfo              = 0xE50;   // AimingInfo Pointer
        constexpr uintptr_t Velocity             = 0xF0;    // Vector3
        constexpr uintptr_t IsGrounded           = 0x11C;   // Boolean
        constexpr uintptr_t FOVOffset            = 0x84;    // Single
        constexpr uintptr_t Weapon               = 0x600;   // WeaponOnHand
        constexpr uintptr_t ColWrite             = 0x80;    // Collider Write
        constexpr uintptr_t DeathInfo            = 0x24A8;  // DeathInfo
    }

// Weapon Data Fields
    namespace Weapon {
        constexpr uintptr_t Data                 = 0x80;
        constexpr uintptr_t Recoil               = 0x18;
    }

// Avatar Visibility
    namespace Avatar {
        constexpr uintptr_t Uma                  = 0x138;
        constexpr uintptr_t Uma_Visible          = 0x101;
        constexpr uintptr_t Uma_Data             = 0x28;
        constexpr uintptr_t UmaData_LocalPlayer  = 0x80;    // Boolean: UMAData.isLocalPlayer
        constexpr uintptr_t UmaData_Teammate     = 0x81;    // Boolean: UMAData.isTeammate (Zero overhead filtering)
        constexpr uintptr_t UmaData_Skip         = 0x81;    // Alias for teammate filter
    }

// Camera Chain
    namespace Camera {
        constexpr uintptr_t Follow               = 0x28;
        constexpr uintptr_t IntPtr               = 0x10;
        constexpr uintptr_t Matrix               = 0x100;
    }

// Aim Information
    namespace Aim {
        constexpr uintptr_t Direction            = 0x40;    // Vector3
        constexpr uintptr_t StartPosition        = 0x4C;    // Vector3
    }

// Skeleton Bones (ITransformNode offsets inside Player)
    namespace Bone {
        constexpr uintptr_t Head                 = 0x6A0;
        constexpr uintptr_t Hip                  = 0x6A8;
        constexpr uintptr_t Breast               = 0x6B0;
        constexpr uintptr_t Root                 = 0x6C8;
        constexpr uintptr_t LeftShoulder         = 0x708;
        constexpr uintptr_t RightShoulder        = 0x710;
        constexpr uintptr_t LeftAnkle            = 0x6D8;
        constexpr uintptr_t RightAnkle           = 0x6E0;
        constexpr uintptr_t LeftToe              = 0x6E8;
        constexpr uintptr_t RightToe             = 0x6F0;
        constexpr uintptr_t RightForeArm         = 0x728;
        constexpr uintptr_t LeftForeArm          = 0x730;
        constexpr uintptr_t RightHand            = 0x718;
        constexpr uintptr_t LeftHand             = 0x720;
    }

// Loot & Drop Items
    namespace Loot {
        constexpr uintptr_t DropObjects          = 0x18;
        constexpr uintptr_t MetaObjects          = 0x10;
        constexpr uintptr_t SpeedObjects         = 0x20;
        constexpr uintptr_t DropObject_Guid      = 0x20;
        constexpr uintptr_t DropObject_Model     = 0x28;
        constexpr uintptr_t DropObject_Shadow    = 0x30;
        constexpr uintptr_t DropObject_ResID     = 0x64;
        constexpr uintptr_t DropObject_Position  = 0x68;
        constexpr uintptr_t DropObject_Rotation  = 0x74;
        constexpr uintptr_t DropObject_Scale     = 0x84;
        constexpr uintptr_t DropObject_Landed    = 0x9C;
    }

// Backwards-Compatible Flat Aliases
    constexpr uintptr_t GameFacade               = Base::GameFacade;
    constexpr uintptr_t GameFacade_P2            = Base::GameFacade_P2;
    constexpr uintptr_t BaseGame_Match           = Base::BaseGame_Match;
    constexpr uintptr_t BaseGame_Timer           = Base::BaseGame_Timer;
    constexpr uintptr_t Timer_Delta              = Base::Timer_Delta;
    constexpr uintptr_t MatchIsRunning           = Base::MatchIsRunning;
    constexpr uintptr_t Match_PlayerDict         = Base::Match_PlayerDict;
    constexpr uintptr_t Match_LocalPlayer        = Base::Match_LocalPlayer;
    constexpr uintptr_t Match_MetaManager        = Base::Match_MetaManager;

    constexpr uintptr_t Dict_Entries             = Dict::Entries;
    constexpr uintptr_t Dict_Count               = Dict::Count;
    constexpr uintptr_t Dict_ArrayHeader         = Dict::ArrayHeader;
    constexpr uintptr_t Dict_EntryStride         = Dict::EntryStride;
    constexpr uintptr_t Dict_EntryValue          = Dict::EntryValue;

    constexpr uintptr_t Player_Rotation          = Player::Rotation;
    constexpr uintptr_t Player_HeadTF            = Player::HeadTF;
    constexpr uintptr_t Player_FootTF            = Player::FootTF;
    constexpr uintptr_t Player_Camera            = Player::Camera;
    constexpr uintptr_t Player_IsFiring          = Player::IsFiring;
    constexpr uintptr_t Player_HP                = Player::HP;
    constexpr uintptr_t Player_Armor             = Player::Armor;
    constexpr uintptr_t Player_IsDead            = Player::IsDead;
    constexpr uintptr_t Player_Avatar            = Player::Avatar;
    constexpr uintptr_t Player_HedColider        = Player::HeadCollider;
    constexpr uintptr_t Player_IsBot             = Player::IsBot;
    constexpr uintptr_t Player_Name              = Player::Name;
    constexpr uintptr_t Player_Speed             = Player::Speed;
    constexpr uintptr_t Player_AimInfo           = Player::AimInfo;
    constexpr uintptr_t Player_Velocity          = Player::Velocity;
    constexpr uintptr_t Player_IsGrounded        = Player::IsGrounded;
    constexpr uintptr_t Player_FOVOffset         = Player::FOVOffset;
    constexpr uintptr_t Player_Weapon            = Player::Weapon;
    constexpr uintptr_t Player_ColWrite          = Player::ColWrite;
    constexpr uintptr_t Player_DeathInfo         = Player::DeathInfo;

    constexpr uintptr_t Avatar_Uma               = Avatar::Uma;
    constexpr uintptr_t Uma_Visible              = Avatar::Uma_Visible;
    constexpr uintptr_t Uma_Data                 = Avatar::Uma_Data;
    constexpr uintptr_t UmaData_Skip             = Avatar::UmaData_Skip;

    constexpr uintptr_t Camera_Follow            = Camera::Follow;
    constexpr uintptr_t Camera_IntPtr            = Camera::IntPtr;
    constexpr uintptr_t Camera_Matrix            = Camera::Matrix;

    constexpr uintptr_t HP_List                  = 0x10;
    constexpr uintptr_t HP_Entry                 = 0x20;
    constexpr uintptr_t HP_Value                 = 0x18;

    constexpr uintptr_t Weapon_Data              = Weapon::Data;
    constexpr uintptr_t Weapon_Recoil            = Weapon::Recoil;

    constexpr uintptr_t AimInfo_Direction        = Aim::Direction;
    constexpr uintptr_t AimInfo_StartPosition    = Aim::StartPosition;

    constexpr uintptr_t Bone_Head                = Bone::Head;
    constexpr uintptr_t Bone_Hip                 = Bone::Hip;
    constexpr uintptr_t Bone_Breast              = Bone::Breast;
    constexpr uintptr_t Bone_Root                = Bone::Root;
    constexpr uintptr_t Bone_LeftShoulder        = Bone::LeftShoulder;
    constexpr uintptr_t Bone_RightShoulder       = Bone::RightShoulder;
    constexpr uintptr_t Bone_LeftAnkle           = Bone::LeftAnkle;
    constexpr uintptr_t Bone_RightAnkle          = Bone::RightAnkle;
    constexpr uintptr_t Bone_LeftToe             = Bone::LeftToe;
    constexpr uintptr_t Bone_RightToe            = Bone::RightToe;
    constexpr uintptr_t Bone_RightForeArm        = Bone::RightForeArm;
    constexpr uintptr_t Bone_LeftForeArm         = Bone::LeftForeArm;
    constexpr uintptr_t Bone_RightHand           = Bone::RightHand;
    constexpr uintptr_t Bone_LeftHand            = Bone::LeftHand;

    constexpr uintptr_t MetaManager_DropObjects  = Loot::DropObjects;
    constexpr uintptr_t MetaManager_MetaObjects  = Loot::MetaObjects;
    constexpr uintptr_t MetaManager_SpeedObjects = Loot::SpeedObjects;
    constexpr uintptr_t DropObject_Guid          = Loot::DropObject_Guid;
    constexpr uintptr_t DropObject_Model         = Loot::DropObject_Model;
    constexpr uintptr_t DropObject_Shadow        = Loot::DropObject_Shadow;
    constexpr uintptr_t DropObject_ResID         = Loot::DropObject_ResID;
    constexpr uintptr_t DropObject_Position      = Loot::DropObject_Position;
    constexpr uintptr_t DropObject_Rotation      = Loot::DropObject_Rotation;
    constexpr uintptr_t DropObject_Scale         = Loot::DropObject_Scale;
    constexpr uintptr_t DropObject_Landed        = Loot::DropObject_Landed;

    constexpr uintptr_t Head                     = Bone_Head;
    constexpr uintptr_t Hip                      = Bone_Hip;
    constexpr uintptr_t Root                     = Bone_LeftToe;

} // namespace Offsets
