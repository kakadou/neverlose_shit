#include "animation_system.h"
#include "../misc/logs.h"

void resolver::initialize(player_t* e, adjust_data* record, adjust_data* previous_record, const float& goal_feet_yaw, const float& pitch)
{
    player = e;

    player_record = record;
    previous_player_record = previous_record;

    original_goal_feet_yaw = goal_feet_yaw;
    original_pitch = pitch;
}

void resolver::reset()
{
    player = nullptr;

    player_record = nullptr;
    previous_player_record = nullptr;

    original_goal_feet_yaw = 0.0f;
    original_pitch = 0.0f;

    bruteforce_ticks = 0;
}

void resolver::animation_resolver(player_t* player, adjust_data* record, adjust_data* previous_record)
{
    if (player && record && previous_record)
    {
        if (player->m_fFlags() & FL_ONGROUND)
        {
            float speed = player->m_vecVelocity().Length2D();

            if (speed <= 9.9)
            {
                if (record->layers[3].m_flWeight == 0.0 && record->layers[3].m_flCycle == 0.0 && record->layers[6].m_flWeight == 0.0)
                {
                    float stand_delta = 0.0;

                    if (player->sequence_activity(record->layers[3].m_nSequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST)
                    {
                        stand_delta = math::angle_difference(record->angles.y, record->lby);

                        record->type = ANIMATION;
                        record->side = stand_delta < 0.0 ? RESOLVER_SECOND : RESOLVER_FIRST;
                        record->high_desync_resolver_active = true;
                    }
                    else
                    {
                        stand_delta = math::angle_difference(record->angles.y, zero_feet_yaw);

                        record->type = ANIMATION;
                        record->side = stand_delta < 0.0 ? RESOLVER_SECOND : RESOLVER_FIRST;
                        record->high_desync_resolver_active = false;
                    }
                }
            }
            else if (speed >= 10.0 || int(record->layers[6].m_flWeight * 1000.0) == int(previous_record->layers[6].m_flWeight * 1000.0))
            {
                const float delta_left = abs(record->layers[6].m_flPlaybackRate - record->resolver_layers[1][6].m_flPlaybackRate);
                const float delta_right = abs(record->layers[6].m_flPlaybackRate - record->resolver_layers[2][6].m_flPlaybackRate);
                const float delta_low_right = abs(record->layers[6].m_flPlaybackRate - record->resolver_layers[3][6].m_flPlaybackRate);
                const float delta_center = abs(record->layers[6].m_flPlaybackRate - record->resolver_layers[0][6].m_flPlaybackRate);

                if (!(delta_center * 1000.0))
                    record->moving_resolver_active = true;

                record->high_desync_resolver_active = false;
                record->side = RESOLVER_ORIGINAL;

                float delta_last = abs(record->layers[6].m_flPlaybackRate - record->resolver_layers[0][6].m_flPlaybackRate);

                if ((delta_low_right * 1000.0) || delta_center < delta_low_right)
                {
                    record->moving_resolver_active = true;
                }
                else
                {
                    record->moving_resolver_active = true;
                    record->side = RESOLVER_LOW_FIRST;
                    
                    delta_last = delta_low_right;
                }

                if (!(delta_left * 1000.0) && delta_last >= delta_left)
                {
                    record->moving_resolver_active = true;
                    record->side = RESOLVER_FIRST;

                    delta_last = delta_left;
                }

                if (!(delta_right * 1000.0) && delta_last >= delta_right)
                {
                    record->moving_resolver_active = true;
                    record->side = RESOLVER_SECOND;
                    return;
                }
            }
        }
    }
}

void resolver::yaw_bruteforce()
{
    resolver_side side = RESOLVER_ORIGINAL;
    resolver_type type = ORIGINAL;

    bool use_high_delta = false;

    int missed_shots = 0;

    if (player && player_record && previous_player_record)
    {
        if ((player->m_fFlags() & FL_ONGROUND) && (player_record->flags & FL_ONGROUND))
        {
            side = player_record->side;
            type = player_record->type;

            missed_shots = g_ctx.globals.missed_shots[player->EntIndex()];

            if (missed_shots > 0 && bruteforce_ticks <= 10)
            {
                switch (side)
                {
                case RESOLVER_FIRST:
                    side = RESOLVER_SECOND;
                    bruteforce_ticks += 1;
                    break;
                case RESOLVER_SECOND:
                    side = RESOLVER_FIRST;
                    bruteforce_ticks += 1;
                    break;
                case RESOLVER_LOW_FIRST:
                    side = RESOLVER_LOW_SECOND;
                    bruteforce_ticks += 1;
                    break;
                case RESOLVER_LOW_SECOND:
                    side = RESOLVER_LOW_FIRST;
                    bruteforce_ticks += 1;
                    break;
                }

                if (bruteforce_ticks > 2 && missed_shots > 2)
                {
                    switch (side)
                    {
                    case RESOLVER_FIRST:
                        side = RESOLVER_LOW_FIRST;
                        bruteforce_ticks += 1;
                        break;
                    case RESOLVER_SECOND:
                        side = RESOLVER_LOW_SECOND;
                        bruteforce_ticks += 1;
                        break;
                    case RESOLVER_LOW_FIRST:
                        side = RESOLVER_FIRST;
                        bruteforce_ticks += 1;
                        break;
                    case RESOLVER_LOW_SECOND:
                        side = RESOLVER_SECOND;
                        bruteforce_ticks += 1;
                        break;
                    }
                }
            }

            player_record->side = side;
            player_record->high_desync_resolver_active = use_high_delta;
        }
    }
}

void resolver::get_freestand_data(resolver_freestand& data)
{
    if (player_record && previous_player_record && player)
    {
        auto head_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.third), player);
        auto body_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_STOMACH, player_record->matrixes_data.third), player);
        auto left_leg_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_LEFT_FOOT, player_record->matrixes_data.third), player);
        auto right_leg_data = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_RIGHT_FOOT, player_record->matrixes_data.third), player);

        if (head_data.valid && body_data.valid && left_leg_data.valid && right_leg_data.valid)
        {
            data.data_available = true;

            if (right_leg_data.visible || left_leg_data.visible)
                data.visible_leg = true;
            else
                data.visible_leg = false;

            if (body_data.visible)
                data.visible_leg = true;
            else
                data.visible_leg = false;

            if (head_data.visible)
                data.visible_leg = true;
            else
                data.visible_leg = false;

            if (data.visible_head && !data.visible_body && !data.visible_leg)
                data.can_use_anti_freestand = true;
            else if (!data.visible_head && data.visible_body && !data.visible_leg)
                data.can_use_anti_freestand = true;
            else if (!data.visible_head && !data.visible_body && data.visible_leg)
                data.can_use_anti_freestand = true;
            else if (data.visible_head && data.visible_leg && !data.visible_body)
                data.can_use_anti_freestand = true;
            else
                data.can_use_anti_freestand = false;
        }
        else
        {
            data.data_available = false;
            data.can_use_anti_freestand = false;
        }
    }
    else
    {
        data.data_available = false;
        data.can_use_anti_freestand = false;
    }
}

void resolver::run_antifreestand()
{
    bool autowalled = false;

    bool hit_first_side = false;
    bool hit_second_side = false;

    float angle_to_local = math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y;
    Vector viewpoint = g_ctx.local()->m_vecOrigin() + Vector(0, 0, 90);

    Vector2D side_first = { (45 * sin(DEG2RAD(angle_to_local))),(45 * cos(DEG2RAD(angle_to_local))) };
    Vector2D side_second = { (45 * sin(DEG2RAD(angle_to_local + 180))) ,(45 * cos(DEG2RAD(angle_to_local + 180))) };

    Vector2D side_third = { (50 * sin(DEG2RAD(angle_to_local))),(50 * cos(DEG2RAD(angle_to_local))) };
    Vector2D side_fourth = { (50 * sin(DEG2RAD(angle_to_local + 180))) ,(50 * cos(DEG2RAD(angle_to_local + 180))) };

    Vector origin = player->m_vecOrigin();

    Vector2D origin_left_right[] = { Vector2D(side_first.x, side_first.y), Vector2D(side_second.x, side_second.y) };
    Vector2D origin_left_right_local[] = { Vector2D(side_third.x, side_third.y), Vector2D(side_fourth.x, side_fourth.y) };

    for (int side = 0; side < 2; side++)
    {
        Vector origin_autowall = { origin.x + origin_left_right[side].x,  origin.y - origin_left_right[side].y , origin.z + 90 };
        Vector viewpoint_autowall = { viewpoint.x + origin_left_right[side].x,  viewpoint.y - origin_left_right_local[side].y , viewpoint.z };

        if (autowall::get().wall_penetration(origin_autowall, viewpoint, player).visible)
        {
            if (side == 0)
            {
                hit_first_side = true;
                freestand_side = 1;
            }
            else if (side == 1)
            {
                hit_second_side = true;
                freestand_side = -1;
            }

            autowalled = true;
        }
        else
        {
            for (int alternative_side = 0; alternative_side < 2; alternative_side++)
            {
                Vector alternative_viewpoint = { origin.x + origin_left_right[alternative_side].x,  origin.y - origin_left_right[alternative_side].y , origin.z + 90 };

                if (autowall::get().wall_penetration(alternative_viewpoint, viewpoint_autowall, player).visible)
                {
                    if (alternative_side == 0)
                    {
                        hit_first_side = true;
                        freestand_side = 1;
                    }
                    else if (alternative_side == 1)
                    {
                        hit_second_side = true;
                        freestand_side = -1;
                    }

                    autowalled = true;
                }
            }
        }
    }
}

void resolver::apply_antifreestand()
{
    resolver_side side = RESOLVER_ORIGINAL;

    if (player->m_fFlags() & FL_ONGROUND)
    {
        float forward_yaw = math::normalize_yaw((math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y) - 180.f);

        float left_yaw = math::normalize_yaw((math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y) - 90.f);
        float right_yaw = math::normalize_yaw((math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y) + 90.f);

        bool sideways = fabsf(math::normalize_yaw(player->get_animation_state()->m_flEyeYaw - left_yaw)) < 45.f || fabsf(math::normalize_yaw(player->get_animation_state()->m_flEyeYaw - right_yaw)) < 45.f;
        bool forward = fabsf(math::normalize_yaw(player->get_animation_state()->m_flEyeYaw - forward_yaw)) < 90.f && !sideways;

        if (forward)
            freestand_side = -freestand_side;

        side = freestand_side > 0 ? RESOLVER_FIRST : RESOLVER_SECOND;
    }

    player_record->side = side;
}

float resolver::resolve_pitch()
{
    return original_pitch;
}