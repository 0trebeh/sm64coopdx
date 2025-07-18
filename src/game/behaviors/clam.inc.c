// clam.inc.c

struct ObjectHitbox sClamShellHitbox = {
    .interactType = INTERACT_CLAM_OR_BUBBA,
    .downOffset = 0,
    .damageOrCoinValue = 2,
    .health = 99,
    .numLootCoins = 0,
    .radius = 150,
    .height = 80,
    .hurtboxRadius = 150,
    .hurtboxHeight = 80,
};

void clam_act_0(void) {
    struct Object* player = nearest_player_to_object(o);
    s32 distanceToPlayer = dist_between_objects(o, player);

    if (cur_obj_init_anim_check_frame(0, 25)) {
        cur_obj_play_sound_2(SOUND_GENERAL_CLAM_SHELL3);
        spawn_mist_from_global();
        cur_obj_become_tangible();

        o->oClamUnkF4 = 10;
        o->oTimer = 0;
        if (sync_object_is_owned_locally(o->oSyncID)) { network_send_object(o); }
    } else if (o->oTimer > 150 && player == gMarioStates[0].marioObj && distanceToPlayer < 500.0f) {
        cur_obj_play_sound_2(SOUND_GENERAL_CLAM_SHELL2);
        o->oAction = 1;
        if (sync_object_is_owned_locally(o->oSyncID)) { network_send_object(o); }
    } else if (o->oClamUnkF4 != 0) {
        o->oClamUnkF4 -= 1;
        cur_obj_shake_y(3.0f);
    }
}

void clam_act_1(void) {
    s16 val06;
    s16 val04;
    s16 val02;

    struct Object* player = nearest_player_to_object(o);
    if (o->oTimer > 150 && player == gMarioStates[0].marioObj) {
        o->oAction = 0;
        if (sync_object_is_owned_locally(o->oSyncID)) { network_send_object(o); }
    } else if (obj_is_rendering_enabled() && cur_obj_init_anim_check_frame(1, 8)) {
        for (val06 = -0x2000; val06 < 0x2000; val06 += 0x555) {
            val04 = (s16)(100.0f * sins(val06));
            val02 = (s16)(100.0f * coss(val06));

            spawn_object_relative(0, val04, 30, val02, o, MODEL_BUBBLE, bhvBubbleMaybe);
        }
    } else if (cur_obj_check_anim_frame(30)) {
        cur_obj_become_intangible();
    }
}

void bhv_clam_loop(void) {
    if (!sync_object_is_initialized(o->oSyncID)) {
        sync_object_init(o, SYNC_DISTANCE_ONLY_EVENTS);
        sync_object_init_field(o, &o->oAction);
        sync_object_init_field(o, &o->oTimer);
        sync_object_init_field(o, &o->oClamUnkF4);
    }
    o->header.gfx.scale[1] = 1.5f;

    switch (o->oAction) {
        case 0:
            clam_act_0();
            break;
        case 1:
            clam_act_1();
            break;
    }

    obj_check_attacks(&sClamShellHitbox, o->oAction);
}
