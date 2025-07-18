static struct ObjectHitbox sKleptoHitbox = {
    .interactType = INTERACT_HIT_FROM_BELOW,
    .downOffset = 0,
    .damageOrCoinValue = 0,
    .health = 1,
    .numLootCoins = 0,
    .radius = 160,
    .height = 250,
    .hurtboxRadius = 80,
    .hurtboxHeight = 200,
};

static Vec3f sKleptoTargetPositions[] = {
    { 2200.0f, 1250.0f, -2820.0f },
    { -6200.0f, 1250.0f, -2800.0f },
    { -6200.0f, 1250.0f, 1150.0f },
};

static u8 sKleptoAttackHandlers[] = { 2, 2, 5, 5, 2, 2 };

static void klepto_target_mario(void) {
    struct MarioState* marioState = nearest_mario_state_to_object(o);
    if (!marioState) { return; }
    struct Object* player = marioState->marioObj;
    s32 angleToPlayer = obj_angle_to_object(o, player);
    o->oKleptoDistanceToTarget = lateral_dist_between_objects(player, o);
    o->oKleptoUnk1B0 = obj_turn_pitch_toward_mario(marioState, 250.0f, 0);
    o->oKleptoYawToTarget = angleToPlayer;
    o->oKleptoUnk1AE = -60;
}

static s32 klepto_set_and_check_if_anim_at_end(void) {
    if (o->oSoundStateID == 6) {
        cur_obj_set_anim_if_at_end(5);
    } else if (o->oSoundStateID == 5) {
        if (cur_obj_set_anim_if_at_end(0)) {
            cur_obj_play_sound_2(SOUND_GENERAL_SWISH_WATER);
            o->header.gfx.animInfo.animFrame = 9;
        }
    } else {
        if (cur_obj_check_anim_frame(9)) {
            cur_obj_play_sound_2(SOUND_GENERAL_SWISH_WATER);
        }
        return TRUE;
    }

    return FALSE;
}

static void klepto_anim_dive(void) {
    if (o->oKleptoUnk1AE > 0) {
        if (o->oKleptoUnk1B0 < -400) {
            o->oKleptoUnk1AE = 0;
        } else {
            if (o->oSoundStateID == 0) {
                if (cur_obj_check_anim_frame(9)) {
                    cur_obj_play_sound_2(SOUND_GENERAL_SWISH_WATER);
                    cur_obj_init_animation_with_sound(6);
                }
            } else if (--o->oKleptoUnk1AE == 0) {
                o->oKleptoUnk1AE = -random_linear_offset(60, 60);
            }

            obj_move_pitch_approach(400, 10);
        }
    } else {
        obj_move_pitch_approach(o->oKleptoUnk1B0, 600);
        if (klepto_set_and_check_if_anim_at_end() != 0) {
            if (o->oKleptoUnk1AE != 0) {
                o->oKleptoUnk1AE += 1;
            } else if (o->oKleptoUnk1B0 > -100) {
                o->oKleptoUnk1AE = random_linear_offset(60, 60);
            }
        }
    }

    obj_face_pitch_approach(0, 1000);
}

static s32 kleptoCachedAnimState = 0;

static void bhv_klepto_on_received_pre(UNUSED u8 localIndex) {
    kleptoCachedAnimState = o->oAnimState;
}

static void bhv_klepto_on_received_post(UNUSED u8 localIndex) {
    if (kleptoCachedAnimState == KLEPTO_ANIM_STATE_HOLDING_NOTHING && o->oAnimState == KLEPTO_ANIM_STATE_HOLDING_STAR) {
        o->oAnimState = KLEPTO_ANIM_STATE_HOLDING_NOTHING;
    }
}

void bhv_klepto_init(void) {
    if (o->oBehParams2ndByte != 0) {
        o->oAnimState = KLEPTO_ANIM_STATE_HOLDING_STAR;
    } else {
        o->oKleptoStartPosX = o->oPosX;
        o->oKleptoStartPosY = o->oPosY;
        o->oKleptoStartPosZ = o->oPosZ;

        if (gMarioStates[0].cap & SAVE_FLAG_CAP_ON_KLEPTO) {
            o->oAnimState = KLEPTO_ANIM_STATE_HOLDING_CAP;
        } else {
            o->oAction = KLEPTO_ACT_WAIT_FOR_MARIO;
        }
    }

    struct SyncObject* so = sync_object_init(o, 4000.0f);
    if (so) {
        so->on_received_pre = bhv_klepto_on_received_pre;
        so->on_received_post = bhv_klepto_on_received_post;
        sync_object_init_field(o, &o->oAnimState);
        sync_object_init_field(o, &o->oFlags);
        sync_object_init_field(o, &o->oKleptoDistanceToTarget);
        sync_object_init_field(o, &o->oKleptoUnkF8);
        sync_object_init_field(o, &o->oKleptoUnkFC);
        sync_object_init_field(o, &o->oKleptoSpeed);
        sync_object_init_field(o, &o->oKleptoTimeUntilTargetChange);
        sync_object_init_field(o, &o->oKleptoTargetNumber);
        sync_object_init_field(o, &o->oKleptoUnk1B0);
        sync_object_init_field(o, &o->oSoundStateID);
        sync_object_init_field(o, &o->oHomeX);
        sync_object_init_field(o, &o->oHomeY);
        sync_object_init_field(o, &o->oHomeZ);
        sync_object_init_field(o, &o->oMoveAnglePitch);
        sync_object_init_field(o, &o->oGravity);
        sync_object_init_field_with_size(o, &o->globalPlayerIndex, 8);
    }
}

static void klepto_change_target(void) {
    s32 newTarget = 0;

    struct Object* player = nearest_player_to_object(o);
    s32 distanceToPlayer = player ? dist_between_objects(o, player) : 10000;

    if (distanceToPlayer > 2000.0f) {
        f32 minTargetDist = 99999.0f;

        for (s32 i = 0; i < 3; i++) {
            f32 dx = player ? player->oPosX - sKleptoTargetPositions[i][0] : 10000;
            f32 dz = player ? player->oPosZ - sKleptoTargetPositions[i][2] : 10000;

            f32 targetDist = sqrtf(dx * dx + dz * dz);
            if (targetDist < minTargetDist) {
                minTargetDist = targetDist;
                newTarget = i;
            }
        }
    } else {
        newTarget = random_u16() % 3;
    }

    o->oKleptoUnkF8 = 400 * absi(newTarget - o->oKleptoTargetNumber);
    o->oKleptoTargetNumber = newTarget;

    o->oHomeX = sKleptoTargetPositions[o->oKleptoTargetNumber][0];
    o->oHomeY = sKleptoTargetPositions[o->oKleptoTargetNumber][1] + o->oKleptoUnkF8;
    o->oHomeZ = sKleptoTargetPositions[o->oKleptoTargetNumber][2];

    o->oKleptoUnkFC = cur_obj_lateral_dist_to_home() / 2;
}

static void klepto_circle_target(f32 radius, f32 targetSpeed) {
    struct Object* player = nearest_player_to_object(o);
    s32 distanceToPlayer = player ? dist_between_objects(o, player) : 10000;

    if (o->oAnimState != KLEPTO_ANIM_STATE_HOLDING_NOTHING
        && ((o->oTimer > 60 && distanceToPlayer > 2000.0f)
            || o->oTimer >= o->oKleptoTimeUntilTargetChange)) {
        klepto_change_target();
        o->oKleptoTimeUntilTargetChange = random_linear_offset(300, 300);
        o->oAction = KLEPTO_ACT_APPROACH_TARGET_HOLDING;
    } else {
        s16 turnAmount = 0x4000 - atan2s(radius, o->oKleptoDistanceToTarget - radius);
        f32 accel = 0.05f;
        if ((s16)(o->oMoveAngleYaw - o->oKleptoYawToTarget) < 0) {
            turnAmount = -turnAmount;
        }

        o->oKleptoYawToTarget += turnAmount;
        klepto_anim_dive();

        //! The multiplied value is sometimes out of range for an s16 during the s32 -> s16 cast,
        //  which might invert sign.
        turnAmount =
            (s16)(s32)(abs_angle_diff(o->oKleptoYawToTarget, o->oMoveAngleYaw) * (0.03f * o->oKleptoSpeed));
        clamp_s16(&turnAmount, 400, 700);
        obj_rotate_yaw_and_bounce_off_walls(o->oKleptoYawToTarget, turnAmount);

        if (o->oKleptoSpeed > 50.0f) {
            accel = 2.0f;
        }

        approach_f32_ptr(&o->oKleptoSpeed, targetSpeed, accel);
    }
}

static void klepto_approach_target(f32 targetSpeed) {
    if (o->oKleptoDistanceToTarget < 1800.0f) {
        o->oAction = KLEPTO_ACT_CIRCLE_TARGET_HOLDING;
    } else {
        if (o->oKleptoUnkFC > 0.0f) {
            if ((o->oKleptoUnkFC -= o->oForwardVel) <= 0.0f) {
                o->oHomeY -= o->oKleptoUnkF8;
            }
        }

        klepto_anim_dive();
        obj_rotate_yaw_and_bounce_off_walls(o->oKleptoYawToTarget, 400);
        approach_f32_ptr(&o->oKleptoSpeed, targetSpeed, 0.05f);
    }
}

static void klepto_act_wait_for_mario(void) {
    if (o->oKleptoDistanceToTarget < 1000.0f) {
        klepto_target_mario();
        if (o->oKleptoDistanceToTarget < 1000.0f) {
            o->oAction = KLEPTO_ACT_TURN_TOWARD_MARIO;
            o->oFlags &= ~0x00000008;
        }
    }

    klepto_circle_target(300.0f, 40.0f);
}

static void klepto_act_turn_toward_mario(void) {
    klepto_target_mario();

    struct Object* player = nearest_player_to_object(o);
    s32 angleToPlayer = player ? obj_angle_to_object(o, player) : 0;

    if (klepto_set_and_check_if_anim_at_end() && cur_obj_check_if_at_animation_end() && o->oKleptoDistanceToTarget > 800.0f
        && abs_angle_diff(angleToPlayer, o->oFaceAngleYaw) < 0x800 && o->oKleptoUnk1B0 < 0x400) {
        cur_obj_play_sound_2(SOUND_OBJ_KLEPTO1);
        o->oAction = KLEPTO_ACT_DIVE_AT_MARIO;
        o->oMoveAngleYaw = o->oFaceAngleYaw;
        o->oFlags |= 0x00000008;

        cur_obj_init_animation_with_sound(3);
    }

    klepto_circle_target(1000.0f, 40.0f);
    obj_face_yaw_approach(angleToPlayer, 1000);
}

static void klepto_act_dive_at_mario(void) {
    struct MarioState* marioState = nearest_mario_state_to_object(o);
    struct Object* player = marioState ? marioState->marioObj : NULL;
    s32 distanceToPlayer = player ? dist_between_objects(o, player) : 10000;
    s32 angleToPlayer = player ? obj_angle_to_object(o, player) : 0;

    approach_f32_ptr(&o->oKleptoSpeed, 60.0f, 10.0f);
    if (o->oSoundStateID == 2) {
        if (cur_obj_check_anim_frame(11)) {
            cur_obj_play_sound_2(SOUND_GENERAL_SWISH_WATER);
        }

        if (cur_obj_set_anim_if_at_end(0)) {
            if (o->oAnimState != KLEPTO_ANIM_STATE_HOLDING_NOTHING) {
                o->oAction = KLEPTO_ACT_CIRCLE_TARGET_HOLDING;
                o->oKleptoTimeUntilTargetChange = 0;
            } else {
                o->oAction = KLEPTO_ACT_WAIT_FOR_MARIO;
            }
        }
    } else {
        f32 dy = player ? (o->oPosY - player->oPosY) : (o->oPosY);
        if (o->oSoundStateID == 3) {
            cur_obj_set_anim_if_at_end(4);
        } else if (o->oVelY > 0.0f && dy > 200.0f) {
            cur_obj_init_animation_with_sound(2);
        }

        o->oKleptoUnk1B0 = -0x3000;
        if (o->oAnimState == KLEPTO_ANIM_STATE_HOLDING_NOTHING) {
            if (o->oSubAction == 0) {
                if (marioState) {
                    o->oKleptoUnk1B0 = obj_turn_pitch_toward_mario(marioState, 0.0f, 0);
                }
                o->oKleptoYawToTarget = angleToPlayer;

                if (dy < 160.0f) {
                    o->oSubAction += 1;
                }
            }

            if (marioState
                && marioState->action != ACT_SLEEPING
                && !(marioState->action & (ACT_FLAG_SHORT_HITBOX | ACT_FLAG_BUTT_OR_STOMACH_SLIDE))
                && distanceToPlayer < 200.0f && dy > 50.0f && dy < 90.0f) {
                if (sync_object_is_owned_locally(o->oSyncID) && mario_lose_cap_to_enemy(marioState, 1) && marioState->playerIndex == 0) {
                    o->oAnimState = KLEPTO_ANIM_STATE_HOLDING_CAP;
                    o->globalPlayerIndex = gNetworkPlayers[marioState->playerIndex].globalIndex;
                    network_send_object(o);
                }
            }
        }
    }

    obj_move_pitch_approach(o->oKleptoUnk1B0, 600);
    obj_face_pitch_approach(o->oMoveAnglePitch, 600);
    obj_rotate_yaw_and_bounce_off_walls(o->oKleptoYawToTarget, 600);
}

static void klepto_act_struck_by_mario(void) {
    cur_obj_init_animation_with_sound(1);

    obj_face_pitch_approach(0, 800);
    obj_face_yaw_approach(o->oMoveAngleYaw + 0x8000, 800);
    obj_face_roll_approach(0, 800);

    if (cur_obj_check_if_near_animation_end()) {
        o->oAction = KLEPTO_ACT_RETREAT;
        o->oGravity = 0.0f;

        o->oMoveAnglePitch = -obj_get_pitch_from_vel();
        o->oKleptoSpeed = sqrtf(o->oForwardVel * o->oForwardVel + o->oVelY * o->oVelY);

        o->oHomeX = o->oPosX;
        o->oHomeY = o->oPosY + 500.0f;
        o->oHomeZ = o->oPosZ;
    }
}

static void klepto_act_retreat(void) {
    cur_obj_init_animation_with_sound(0);
    approach_f32_ptr(&o->oKleptoSpeed, 40.0f, 10.0f);

    obj_move_pitch_approach(o->oKleptoUnk1B0, 1000);

    obj_face_pitch_approach(o->oMoveAnglePitch, 1000);
    obj_rotate_yaw_and_bounce_off_walls(o->oKleptoYawToTarget, 600);

    if (obj_face_yaw_approach(o->oMoveAngleYaw, 1000)) {
        if (abs_angle_diff(o->oFaceAnglePitch, o->oMoveAnglePitch) == 0) {
            o->oAction = KLEPTO_ACT_RESET_POSITION;
            o->oHomeY = 1500.0f;
            o->oKleptoUnk1AE = -100;
            o->oFlags |= 0x00000008;
            cur_obj_become_tangible();
        }
    }
}

static void klepto_act_reset_position(void) {
    if (o->oTimer < 300) {
        klepto_circle_target(300.0f, 20.0f);
    } else if (o->oBehParams2ndByte != 0) {
        o->oHomeX = -2000.0f;
        o->oHomeZ = -1000.0f;
        o->oHomeY = o->oKleptoDistanceToTarget = 9999.0f;

        if (o->oPosY > 5000.0f) {
            obj_mark_for_deletion(o);
        } else {
            klepto_approach_target(20.0f);
        }
    } else {
        o->oAction = KLEPTO_ACT_WAIT_FOR_MARIO;
        o->oHomeX = o->oKleptoStartPosX;
        o->oHomeY = o->oKleptoStartPosY;
        o->oHomeZ = o->oKleptoStartPosZ;
    }
}

void obj_set_speed_to_zero(void) {
    o->oForwardVel = o->oVelY = 0.0f;
}

void bhv_klepto_update(void) {
    struct MarioState *marioState = nearest_mario_state_to_object(o);
    struct Object *player = marioState ? marioState->marioObj : NULL;
    s32 angleToPlayer = player ? obj_angle_to_object(o, player) : 0;

    cur_obj_update_floor_and_walls();

    o->oKleptoDistanceToTarget = cur_obj_lateral_dist_to_home();
    o->oKleptoUnk1B0 = obj_get_pitch_to_home(o->oKleptoDistanceToTarget);
    o->oKleptoYawToTarget = cur_obj_angle_to_home();

    if (o->oAction == KLEPTO_ACT_STRUCK_BY_MARIO) {
        klepto_act_struck_by_mario();
    } else {
        obj_compute_vel_from_move_pitch(o->oKleptoSpeed);

        switch (o->oAction) {
            case KLEPTO_ACT_CIRCLE_TARGET_HOLDING:
                klepto_circle_target(300.0f, 40.0f);
                break;
            case KLEPTO_ACT_APPROACH_TARGET_HOLDING:
                klepto_approach_target(50.0f);
                break;
            case KLEPTO_ACT_WAIT_FOR_MARIO:
                klepto_act_wait_for_mario();
                break;
            case KLEPTO_ACT_TURN_TOWARD_MARIO:
                klepto_act_turn_toward_mario();
                break;
            case KLEPTO_ACT_DIVE_AT_MARIO:
                klepto_act_dive_at_mario();
                break;
            case KLEPTO_ACT_RESET_POSITION:
                klepto_act_reset_position();
                break;
            case KLEPTO_ACT_RETREAT:
                klepto_act_retreat();
                break;
        }

        if (obj_handle_attacks(&sKleptoHitbox, o->oAction, sKleptoAttackHandlers)) {
            cur_obj_play_sound_2(SOUND_OBJ_KLEPTO2);

            u8 kleptoHoldingCap = (o->oAnimState == KLEPTO_ANIM_STATE_HOLDING_CAP);

            if (sync_object_is_owned_locally(o->oSyncID) && kleptoHoldingCap) {
                struct NetworkPlayer* np = network_player_from_global_index(o->globalPlayerIndex);
                if (np == NULL) { np = gNetworkPlayerLocal; }
                u8 modelIndex = (np->overrideModelIndex < CT_MAX) ? np->overrideModelIndex : 0;
                u32 capModel = gCharacters[modelIndex].capModelId;

                gMarioStates[0].cap &= ~SAVE_FLAG_CAP_ON_KLEPTO;

                struct Object* cap = spawn_object(o, capModel, bhvNormalCap);
                if (cap != NULL) {
                    cap->globalPlayerIndex = o->globalPlayerIndex;

                    struct Object* spawn_objects[] = { cap };
                    u32 models[] = { capModel };
                    network_send_spawn_objects(spawn_objects, models, 1);
                }

            } else if (o->oAnimState == KLEPTO_ANIM_STATE_HOLDING_STAR) {
                f32* starPos = gLevelValues.starPositions.KleptoStarPos;
                struct Object* star = spawn_default_star(starPos[0], starPos[1], starPos[2]);

                // If we're not the closet to Klepto,
                // Don't play the cutscene!
                if (star != NULL && marioState != &gMarioStates[0]) {
                    star->oStarSpawnExtCutsceneFlags = 0;
                }
            }

            if (sync_object_is_owned_locally(o->oSyncID)) {
                o->oAnimState = KLEPTO_ANIM_STATE_HOLDING_NOTHING;
                o->oAction = KLEPTO_ACT_STRUCK_BY_MARIO;
                network_send_object(o);
            }
            o->oGravity = -2.0f;

            o->oMoveAngleYaw = angleToPlayer + 0x8000;
            o->oFlags &= ~0x00000008;
            cur_obj_become_intangible();
        } else if ((marioState && marioState->action == ACT_SLEEPING) || (marioState && marioState->action & (ACT_FLAG_SHORT_HITBOX | ACT_FLAG_BUTT_OR_STOMACH_SLIDE))) {
            cur_obj_become_intangible();
        } else {
            cur_obj_become_tangible();
        }
    }

    obj_roll_to_match_yaw_turn(o->oKleptoYawToTarget, 0x3000, 600);
    cur_obj_move_standard(78);
}
