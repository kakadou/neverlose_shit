// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "animation_system.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];

void lagcompensation::fsn(ClientFrameStage_t stage)
{
	if (stage != FRAME_NET_UPDATE_END)
		return;

	if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled)
		return;

	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == g_ctx.local())
			continue;

		if (!valid(i, e))
			continue;

		auto time_delta = abs(TIME_TO_TICKS(e->m_flSimulationTime()) - m_globals()->m_tickcount);

		if (time_delta > 1.0f / m_globals()->m_intervalpertick)
			continue;

		auto update = player_records[i].empty() || e->m_flSimulationTime() != e->m_flOldSimulationTime(); //-V550

		if (update) //-V550
		{
			if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f)
				for (auto& record : player_records[i])
					record.invalid = true;

			player_records[i].emplace_front(adjust_data(e));

			auto records = &player_records[e->EntIndex()]; //-V826

			if (records->empty())
				continue;

			adjust_data* previous_record = nullptr;

			if (records->size() >= 2)
				previous_record = &records->at(1);

			auto record = &records->front();

			update_player_animations(e);

			while (player_records[i].size() > 32)
				player_records[i].pop_back();
		}
	}
}

bool lagcompensation::valid(int i, player_t* e)
{
	if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled || !e->valid(false))
	{
		if (!e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].reset();

			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
		}
		else if (e->IsDormant())
			is_dormant[i] = true;

		player_records[i].clear();
		return false;
	}

	return true;
}

void lagcompensation::update_player_animations(player_t* e)
{
	auto animstate = e->get_animation_state();

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2)
		previous_record = &records->at(1);

	auto record = &records->front();

	AnimationLayer animlayers[13];

	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();

	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;

	player_resolver[e->EntIndex()].zero_feet_yaw = animstate->m_flGoalFeetYaw;

	if (previous_record)
	{
		auto velocity = e->m_vecVelocity();
		auto was_in_air = e->m_fFlags() & FL_ONGROUND && previous_record->flags & FL_ONGROUND;

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[6].m_flWeight >= 0.1f)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = 260.0f;
					auto weapon_info = e->m_hActiveWeapon().Get()->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					float weight = animlayers[6].m_flWeight;
					float previous_weight = previous_record->layers[6].m_flWeight;
					float loop_weight = animlayers[11].m_flWeight;

					float length_2d = record->velocity.Length2D();

					if ((loop_weight <= 0.f || loop_weight >= 1.f) && length_2d >= 0.1f)
					{
						bool valid_6th_layer = weight > 0.f && weight < 1.f && (weight >= previous_weight);

						if (valid_6th_layer)
						{
							float closest_speed_to_minimal = max_speed * 0.34f;
							float speed_multiplier = std::fmaxf(0.f, (max_speed * 0.52f) - (max_speed * 0.34f));

							auto v208 = 1.f - record->duck_amount;
							auto speed_via_6th_layer = (((v208 * speed_multiplier) + closest_speed_to_minimal) * weight) / length_2d;

							animation_speed = speed_via_6th_layer;
						}
					}
				}
			}

			if (animation_speed > 0.0f)
			{
				animation_speed /= e->m_vecVelocity().Length2D();

				e->m_vecVelocity().x *= animation_speed;
				e->m_vecVelocity().y *= animation_speed;
			}

			if (records->size() >= 3 && time_difference > m_globals()->m_intervalpertick)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto average_direction = current_direction - previous_direction;
					average_direction = DEG2RAD(math::normalize_yaw(current_direction + average_direction * 0.5f));

					auto direction_cos = cos(average_direction);
					auto dirrection_sin = sin(average_direction);

					auto velocity_speed = e->m_vecVelocity().Length2D();

					e->m_vecVelocity().x = direction_cos * velocity_speed;
					e->m_vecVelocity().y = dirrection_sin * velocity_speed;
				}
			}

			if (!(e->m_fFlags() & FL_ONGROUND))
			{
				static auto sv_gravity = m_cvar()->FindVar(crypt_str("sv_gravity"));

				auto fixed_timing = math::clamp(time_difference, m_globals()->m_intervalpertick, 1.0f);
				e->m_vecVelocity().z -= sv_gravity->GetFloat() * fixed_timing * 0.5f;
			}
			else
				e->m_vecVelocity().z = 0.0f;
		}
	}

	e->m_iEFlags() &= ~1800;

	if (e->m_fFlags() & FL_ONGROUND && e->m_vecVelocity().Length() > 0.0f && animlayers[6].m_flWeight <= 0.0f)
		e->m_vecVelocity().Zero();

	e->m_vecAbsVelocity() = e->m_vecVelocity();
	e->m_bClientSideAnimation() = true;

	if (is_dormant[e->EntIndex()])
	{
		is_dormant[e->EntIndex()] = false;

		if (e->m_fFlags() & FL_ONGROUND)
		{
			animstate->m_bOnGround = true;
			animstate->m_bInHitGroundAnimation = false;
		}

		animstate->time_since_in_air() = 0.0f;
		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
	}

	auto updated_animations = false;

	c_baseplayeranimationstate state;
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	if (previous_record)
	{
		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));

		auto ticks_chocked = 1;
		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 31)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			auto land_time = 0.0f;
			auto land_in_cycle = false;
			auto is_landed = false;
			auto on_ground = false;

			if (animlayers[4].m_flCycle < 0.5f && (!(e->m_fFlags() & FL_ONGROUND) || !(previous_record->flags & FL_ONGROUND)))
			{
				land_time = e->m_flSimulationTime() - animlayers[4].m_flPlaybackRate * animlayers[4].m_flCycle;
				land_in_cycle = land_time >= previous_record->simulation_time;
			}

			auto duck_amount_per_tick = (e->m_flDuckAmount() - previous_record->duck_amount) / ticks_chocked;

			for (auto i = 0; i < ticks_chocked; ++i)
			{
				auto lby_delta = fabs(math::normalize_yaw(e->m_flLowerBodyYawTarget() - previous_record->lby));

				if (lby_delta > 0.0f && e->m_vecVelocity().Length() < 5.0f)
				{
					auto delta = ticks_chocked - i;
					auto use_new_lby = true;

					if (lby_delta < 1.0f)
						use_new_lby = !delta; //-V547
					else
						use_new_lby = delta < 2;

					e->m_flLowerBodyYawTarget() = use_new_lby ? backup_lower_body_yaw_target : previous_record->lby;
				}

				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				if (duck_amount_per_tick) //-V550
					e->m_flDuckAmount() = previous_record->duck_amount + duck_amount_per_tick * (float)i;

				on_ground = e->m_fFlags() & FL_ONGROUND;

				if (land_in_cycle && !is_landed)
				{
					if (land_time <= simulated_time)
					{
						is_landed = true;
						on_ground = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				if (on_ground)
					e->m_fFlags() |= FL_ONGROUND;
				else
					e->m_fFlags() &= ~FL_ONGROUND;

				auto simulated_ticks = TIME_TO_TICKS(simulated_time);

				m_globals()->m_realtime = simulated_time;
				m_globals()->m_curtime = simulated_time;
				m_globals()->m_framecount = simulated_ticks;
				m_globals()->m_tickcount = simulated_ticks;
				m_globals()->m_interpolation_amount = 0.0f;

				g_ctx.globals.updating_animation = true;
				e->update_clientside_animation();
				g_ctx.globals.updating_animation = false;

				m_globals()->m_realtime = backup_realtime;
				m_globals()->m_curtime = backup_curtime;
				m_globals()->m_framecount = backup_framecount;
				m_globals()->m_tickcount = backup_tickcount;
				m_globals()->m_interpolation_amount = backup_interpolation_amount;

				updated_animations = true;
			}
		}
	}

	if (!updated_animations)
	{
		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	auto setup_matrix = [&](player_t* e, AnimationLayer* layers, const int& matrix) -> void
	{
		e->invalidate_physics_recursive(8);

		AnimationLayer backup_layers[13];
		memcpy(backup_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		memcpy(e->get_animlayers(), layers, e->animlayer_count() * sizeof(AnimationLayer));

		switch (matrix)
		{
		case MAIN:
			e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
			break;
		case NONE:
			e->setup_bones_fixed(record->matrixes_data.zero, BONE_USED_BY_HITBOX);
			break;
		case FIRST:
			e->setup_bones_fixed(record->matrixes_data.first, BONE_USED_BY_HITBOX);
			break;
		case SECOND:
			e->setup_bones_fixed(record->matrixes_data.second, BONE_USED_BY_HITBOX);
			break;
		case THIRD:
			e->setup_bones_fixed(record->matrixes_data.third, BONE_USED_BY_HITBOX);
			break;
		}

		memcpy(e->get_animlayers(), backup_layers, e->animlayer_count() * sizeof(AnimationLayer));
	};

	if (g_ctx.local()->is_alive() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum() && !g_cfg.legitbot.enabled)
	{
		animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()]; //-V807

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		feet_delta[e->EntIndex()] = math::angle_difference(record->angles.y, animstate->m_flGoalFeetYaw);
		previous_goal_feet_yaw[e->EntIndex()] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y); //-V807

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, NONE);
		memcpy(record->resolver_layers[0], e->get_animlayers(), sizeof(AnimationLayer) * 13);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 60.0f);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, FIRST);
		memcpy(record->resolver_layers[1], e->get_animlayers(), sizeof(AnimationLayer) * 13);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 60.0f);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, SECOND);
		memcpy(record->resolver_layers[2], e->get_animlayers(), sizeof(AnimationLayer) * 13);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 30.0f);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, THIRD);
		memcpy(record->resolver_layers[3], e->get_animlayers(), sizeof(AnimationLayer) * 13);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		player_resolver[e->EntIndex()].initialize(e, record, previous_record, previous_goal_feet_yaw[e->EntIndex()], e->m_angEyeAngles().x);
		player_resolver[e->EntIndex()].animation_resolver(e, record, previous_record);

		player_resolver[e->EntIndex()].get_freestand_data(player_freestand[e->EntIndex()]);

		if (player_freestand[e->EntIndex()].data_available)
			player_resolver[e->EntIndex()].run_antifreestand();

		if (player_freestand[e->EntIndex()].data_available && player_freestand[e->EntIndex()].can_use_anti_freestand)
			player_resolver[e->EntIndex()].apply_antifreestand();

		switch (record->side)
		{
		case RESOLVER_ORIGINAL:
			e->get_animation_state()->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()];
			break;
		case RESOLVER_ZERO:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
			break;
		case RESOLVER_FIRST:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 60.0f);
			break;
		case RESOLVER_SECOND:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 60.0f);
			break;
		case RESOLVER_LOW_FIRST:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 25.0f);
			break;
		case RESOLVER_LOW_SECOND:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 25.0f);
			break;
		case RESOLVER_HIGH_FIRST:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + e->get_max_desync_delta() * 2);
			break;
		case RESOLVER_HIGH_SECOND:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - e->get_max_desync_delta() * 2);
			break;
		}

		if (record->high_desync_resolver_active)
		{
			switch (record->side)
			{
			case RESOLVER_FIRST:
				e->m_angEyeAngles().y = math::normalize_yaw(animstate->m_flEyeYaw - 60.0);
				break;
			case RESOLVER_SECOND:
				e->m_angEyeAngles().y = math::normalize_yaw(animstate->m_flEyeYaw + 60.0);
				break;
			}
		}

		player_resolver[e->EntIndex()].resolved_feet_yaw = animstate->m_flGoalFeetYaw;

		e->m_angEyeAngles().x = player_resolver[e->EntIndex()].resolve_pitch();
	}

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	setup_matrix(e, animlayers, MAIN);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;

	e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
	e->m_flDuckAmount() = backup_duck_amount;
	e->m_fFlags() = backup_flags;
	e->m_iEFlags() = backup_eflags;

	memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(player_resolver[e->EntIndex()].previous_layers, animlayers, sizeof(AnimationLayer) * 13);

	record->store_data(e, false);

	if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
		record->invalid = true;

	e->invalidate_physics_recursive(8);
	e->invalidate_bone_cache();
}

/*
void lagcompensation::update_player_animations(player_t* e, adjust_data* record, adjust_data* previous_record)
{
	float curtime = m_globals()->m_curtime;
	float realtime = m_globals()->m_realtime;
	float frametime = m_globals()->m_frametime;
	float abs_frametime = m_globals()->m_absoluteframetime;
	float interpolation = m_globals()->m_interpolation_amount;
	int framecount = m_globals()->m_framecount;
	int tickcount = m_globals()->m_tickcount;

	int next_simulation_tick = TIME_TO_TICKS(e->m_flSimulationTime()) + 1;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_realtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;
	m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
	m_globals()->m_interpolation_amount = 0.f;
	m_globals()->m_framecount = next_simulation_tick;
	m_globals()->m_tickcount = next_simulation_tick;

	std::memcpy(record->layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

	float max_player_speed = 260.f;

	if (e->m_hActiveWeapon().Get() && e->m_hActiveWeapon().Get()->get_csweapon_info())
		max_player_speed = e->m_bIsScoped() ? e->m_hActiveWeapon().Get()->get_csweapon_info()->flMaxPlayerSpeedAlt : e->m_hActiveWeapon().Get()->get_csweapon_info()->flMaxPlayerSpeed;

	bool is_landed = false;
	bool on_ground = false;

	if (previous_record)
	{
		if (record->layers[6].m_flWeight >= 0.1f)
		{
			float weight = record->layers[6].m_flWeight;
			float previous_weight = previous_record->layers[6].m_flWeight;
			float loop_weight = record->layers[6].m_flWeight;

			float length_2d = record->velocity.Length2D();
			float max_speed = e->m_hActiveWeapon().Get() ? max(max_player_speed, 0.001f) : 260.f;

			if ((loop_weight <= 0.f || loop_weight >= 1.f) && length_2d >= 0.1f)
			{
				bool valid_6th_layer = weight > 0.f && weight < 1.f && (weight >= previous_weight);

				if (valid_6th_layer)
				{
					float closest_speed_to_minimal = max_speed * 0.34f;
					float speed_multiplier = std::fmaxf(0.f, (max_speed * 0.52f) - (max_speed * 0.34f));

					float duck_modifier = 1.f - record->duck_amount;

					float speed_via_6th_layer = (((duck_modifier * speed_multiplier) + closest_speed_to_minimal) * weight) / length_2d;

					e->m_vecVelocity().x *= speed_via_6th_layer;
					e->m_vecVelocity().y *= speed_via_6th_layer;
				}
			}
		}

		float simulated_time = previous_record->simulation_time - (e->m_flSimulationTime() - e->m_flOldSimulationTime());

		if (!(e->m_fFlags() & FL_ONGROUND && record->layers[5].m_flWeight > 0.f && previous_record->layers[5].m_flWeight > 0.f)) 
		{
			int activity = e->sequence_activity(record->layers[5].m_nSequence);

			if (activity == 988 || activity == 989)
			{
				float land_time = record->layers[5].m_flCycle / record->layers[5].m_flPlaybackRate;
				bool land_in_cycle = land_time >= previous_record->simulation_time;

				if (land_in_cycle && !is_landed) 
				{
					if (land_time <= simulated_time)
					{
						on_ground = true;
						is_landed = true;
					}
					else
						on_ground = previous_record->flags & FL_ONGROUND;
				}

				if (on_ground)
					e->m_fFlags() |= FL_ONGROUND;
				else
					e->m_fFlags() &= ~FL_ONGROUND;
			}
		}
	}

	e->m_iEFlags() &= ~1000;
	e->m_vecAbsVelocity() = e->m_vecVelocity();

	e->m_bClientSideAnimation() = true;

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	float backup_feet_yaw = e->get_animation_state()->m_flGoalFeetYaw;

	std::memcpy(e->get_animlayers(), record->layers, e->animlayer_count() * sizeof(AnimationLayer));
	e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y + 60.f);

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	std::memcpy(record->resolver_layers[1], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	e->setup_bones_fixed(record->matrixes_data.first, BONE_USED_BY_HITBOX);

	std::memcpy(e->get_animlayers(), record->layers, e->animlayer_count() * sizeof(AnimationLayer));
	e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y - 60.f);

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	std::memcpy(record->resolver_layers[2], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	e->setup_bones_fixed(record->matrixes_data.second, BONE_USED_BY_HITBOX);

	std::memcpy(e->get_animlayers(), record->layers, e->animlayer_count() * sizeof(AnimationLayer));
	e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y + 30.f);

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	std::memcpy(record->resolver_layers[3], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	e->setup_bones_fixed(record->matrixes_data.third, BONE_USED_BY_HITBOX);

	std::memcpy(e->get_animlayers(), record->layers, e->animlayer_count() * sizeof(AnimationLayer));
	e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y);

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	std::memcpy(record->resolver_layers[0], e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	e->setup_bones_fixed(record->matrixes_data.zero, BONE_USED_BY_HITBOX);

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	player_resolver[e->EntIndex()].initialize(e, record, previous_record, backup_feet_yaw, e->m_angEyeAngles().x);
	player_resolver[e->EntIndex()].animation_resolver();

	player_resolver[e->EntIndex()].yaw_bruteforce();

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
	std::memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	switch (record->side)
	{
	case RESOLVER_ORIGINAL:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y);
		break;
	case RESOLVER_ZERO:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y);
		break;
	case RESOLVER_FIRST:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y + 60.f);
		break;
	case RESOLVER_LOW_FIRST:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y + 25.f);
		break;
	case RESOLVER_SECOND:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y - 60.f);
		break;
	case RESOLVER_LOW_SECOND:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y - 25.f);
		break;
	case RESOLVER_HIGH_FIRST:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y + 90.f);
		break;
	case RESOLVER_HIGH_SECOND:
		e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(record->angles.y - 90.f);
		break;
	}

	g_ctx.globals.updating_animation = true;
	e->update_clientside_animation();
	g_ctx.globals.updating_animation = false;

	m_globals()->m_curtime = curtime;
	m_globals()->m_realtime = realtime;
	m_globals()->m_frametime = frametime;
	m_globals()->m_absoluteframetime = abs_frametime;
	m_globals()->m_interpolation_amount = interpolation;
	m_globals()->m_framecount = framecount;
	m_globals()->m_tickcount = tickcount;

	e->invalidate_physics_recursive(8);
	e->invalidate_bone_cache();
}
*/