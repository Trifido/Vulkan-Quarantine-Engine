#pragma once

#ifndef PHYSICS_TUPES_H
#define PHYSICS_TUPES_H

enum PhysicBodyType
{
    STATIC_BODY = 0,
    RIGID_BODY = 1 << 0,
    KINEMATIC_BODY = 1 << 1,
};

enum CollisionFlag
{
    COL_NOTHING = 0,
    COL_DEFAULT = 1 << 0,
    COL_PLAYER = 1 << 1,
    COL_SCENE = 1 << 2,
    COL_ENEMY = 1 << 3,
    COL_TRIGGER = 1 << 4,
    COL_ALL = -1
};

#endif // !PHYSICS_TUPES_H
