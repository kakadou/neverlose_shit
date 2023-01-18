// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "antiaim.h"
#include "knifebot.h"
#include "zeusbot.h"
#include "..\misc\fakelag.h"
#include "..\misc\prediction_system.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"

void antiaim::create_move(CUserCmd* m_pcmd)
{
	if (!g_cfg.antiaim.enable)
		return;

	if (condition(m_pcmd))
		return;

	if ((g_ctx.globals.exploits || (!g_cfg.antiaim.lby_mode)) && !g_cfg.misc.fast_stop && (!g_ctx.globals.weapon->is_grenade() || !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2)) && engineprediction::get().backup_data.velocity.Length2D() <= 20.0f) //-V648
	{
		auto fakeducking = false;

		if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
			fakeducking = true;

		auto speed = 1.01f;

		if (m_pcmd->m_buttons & IN_DUCK || fakeducking)
			speed *= 2.94117647f;

		if (m_pcmd->m_command_number & 1)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;
	}

	m_pcmd->m_viewangles.x = get_pitch(m_pcmd);
	m_pcmd->m_viewangles.y = get_yaw(m_pcmd);
}

void antiaim::setup_lby_angle(CUserCmd* m_pcmd)
{

}

float antiaim::get_pitch(CUserCmd* m_pcmd)
{
	if (g_cfg.antiaim.pitch_type == 0)
		return m_pcmd->m_viewangles.x;

	if ((m_pcmd->m_buttons & IN_USE))
		return m_pcmd->m_viewangles.x;

	float m_flAngle = 0.0;

	switch (g_cfg.antiaim.pitch_type)
	{
	case 1:
		m_flAngle = 89.0;
		break;
	case 2:
		m_flAngle = -540.0;
		break;
	case 3:
		m_flAngle = 540.0;
		break;
	}

	return m_flAngle;
}

bool is_break()
{
	if (g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands > 12)
		return false;

	if (!g_ctx.globals.fakeducking && m_clientstate()->iChokedCommands > 14)
	{
		g_ctx.send_packet = true;
		fakelag::get().started_peeking = false;
	}

	auto animstate = g_ctx.local()->get_animation_state(); //-V807

	if (!animstate)
		return false;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.0f)
		g_ctx.globals.next_lby_update = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase + 14);
	else
	{
		if (TICKS_TO_TIME(g_ctx.globals.fixed_tickbase) > g_ctx.globals.next_lby_update)
		{
			g_ctx.globals.next_lby_update = 0.0f;
			return true;
		}
	}

	return false;
}

float antiaim::get_yaw(CUserCmd* m_pcmd)
{
	if ((m_pcmd->m_buttons & IN_USE))
		return m_pcmd->m_viewangles.y;

	float m_flAngle = m_pcmd->m_viewangles.y;
	float m_flDesyncAngle = 0.0f;

	m_flDesyncAngle = 0;

	bool side = key_binds::get().get_key_bind_state(16);
	bool flip_desync = false;

	bool flip_jitter = math::random_int(-1, 1);

	if (g_cfg.antiaim.yaw_base == 0)
		m_flAngle += g_cfg.antiaim.yaw_offset;
	else if (g_cfg.antiaim.yaw_base == 1)
		m_flAngle += (180.0 + g_cfg.antiaim.yaw_offset);
	else if (g_cfg.antiaim.yaw_base == 2)
	{
		m_flAngle -= 90.0;
		m_flAngle += (g_cfg.antiaim.yaw_offset);
	}
	else if (g_cfg.antiaim.yaw_base == 3)
		m_flAngle += (90.0 + g_cfg.antiaim.yaw_offset);
	else if (g_cfg.antiaim.yaw_base == 4)
		m_flAngle = (at_targets() + g_cfg.antiaim.yaw_offset);
	else if (g_cfg.antiaim.yaw_base == 5)
	{
		freestanding(m_pcmd);

		m_flAngle += (180.0 + g_cfg.antiaim.yaw_offset);

		if (manual_side == SIDE_RIGHT)
			m_flAngle += 90.0;
		else if (manual_side == SIDE_LEFT)
			m_flAngle -= 90.0;
	}

	if (g_cfg.antiaim.yaw_modifier == 1)
		m_flAngle += g_cfg.antiaim.yaw_jitter;
	else if (g_cfg.antiaim.yaw_modifier == 2)
		m_flAngle += flip_jitter ? -(g_cfg.antiaim.yaw_jitter / 2) : (g_cfg.antiaim.yaw_jitter / 2);
	else if (g_cfg.antiaim.yaw_modifier == 3)
		m_flAngle += math::random_int(0, 180);

	if (g_cfg.antiaim.fake_yaw)
	{
		side = g_cfg.antiaim.inverter;

		if (key_binds::get().get_key_bind_state(16))
			side = !side;

		bool invert_desync_jitter = math::random_int(-1, 1);

		if (g_cfg.antiaim.desync_options[JITTER] || g_cfg.antiaim.desync_options[RANDOMIZE_JITTER])
			side = invert_desync_jitter;

		if (g_cfg.antiaim.desync_options[AVOID_OVERLAP])
		{
			Vector angle_diff = local_animations::get().local_data.real_angles - local_animations::get().local_data.fake_angles;

			if (abs(angle_diff.y) < 20)
				side = !side;
		}

		flip_desync = side;

		flip = side;

		if (g_cfg.antiaim.freestand_desync > 0)
		{
			if (g_cfg.antiaim.freestand_desync == 1)
				side = !automatic_direction();
			else if (g_cfg.antiaim.freestand_desync == 2)
				side = automatic_direction();
		}

		m_flDesyncAngle += side ? -g_cfg.antiaim.invert_desync_angle : g_cfg.antiaim.desync_angle;
		m_flAngle -= m_flDesyncAngle;

		desync_angle = m_flDesyncAngle;

		if (!m_flDesyncAngle)
			return m_flAngle;
	}

	static bool choke = false;
	static bool reverse = false;
	static bool sway = false;

	if (is_break() && g_cfg.antiaim.lby_mode)
	{
		auto speed = 1.01f;

		if (m_pcmd->m_buttons & IN_DUCK || g_ctx.globals.fakeducking)
			speed *= 2.94117647f;

		static auto switch_move = false;

		if (switch_move)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;

		switch_move = !switch_move;

		switch (g_cfg.antiaim.lby_mode) {
		case 1: m_flAngle += side ? -(g_ctx.local()->get_max_desync_delta() * 2) : (g_ctx.local()->get_max_desync_delta() * 2); break; // opposite
		case 2: if (!sway) m_flAngle += side ? -116.f : 116.f; break; // sway
		}

		m_flAngle = math::normalize_yaw(m_flAngle);
		sway = !sway;

		g_ctx.send_packet = false;
		choke = true;

		return m_flAngle;
	}
	else if (choke)
	{
		g_ctx.send_packet = false;
		choke = false;
	}
	else if (g_ctx.send_packet)
		m_flAngle += m_flDesyncAngle;

	return m_flAngle;
}

bool antiaim::condition(CUserCmd* m_pcmd, bool dynamic_check)
{
	if (!m_pcmd)
		return true;

	if (!g_ctx.available())
		return true;

	if (!g_cfg.antiaim.enable)
		return true;

	if (!g_ctx.local()->is_alive()) //-V807
		return true;

	if (g_ctx.local()->m_bGunGameImmunity() || g_ctx.local()->m_fFlags() & FL_FROZEN)
		return true;

	if (g_ctx.local()->get_move_type() == MOVETYPE_NOCLIP || g_ctx.local()->get_move_type() == MOVETYPE_LADDER)
		return true;

	if (g_ctx.globals.aimbot_working)
		return true;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return true;

	if (m_pcmd->m_buttons & IN_ATTACK && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !weapon->is_non_aim())
		return true;

	auto revolver_shoot = weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !g_ctx.globals.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

	if (revolver_shoot)
		return true;

	if ((m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2) && weapon->is_knife())
		return true;

	if (dynamic_check && freeze_check)
		return true;

	if (dynamic_check && weapon->is_grenade() && weapon->m_fThrowTime())
		return true;

	return false;
}

bool antiaim::should_break_lby(CUserCmd* m_pcmd)
{
	if (!g_cfg.antiaim.lby_mode || g_ctx.globals.exploits)
		return false;

	auto animstate = g_ctx.local()->get_animation_state();

	if (!animstate)
		return false;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.0f)
		g_ctx.globals.next_lby_update = m_globals()->m_curtime + 0.22f;
	else if (m_globals()->m_curtime > g_ctx.globals.next_lby_update)
	{
		g_ctx.globals.next_lby_update = m_globals()->m_curtime + 1.1f;
		return true;
	}

	return false;
}

float antiaim::at_targets()
{
	player_t* target = nullptr;
	auto best_fov = FLT_MAX;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e->valid(true))
			continue;

		auto weapon = e->m_hActiveWeapon().Get();

		if (!weapon)
			continue;

		if (weapon->is_non_aim())
			continue;

		Vector angles;
		m_engine()->GetViewAngles(angles);

		auto fov = math::get_fov(angles, math::calculate_angle(g_ctx.globals.eye_pos, e->GetAbsOrigin()));

		if (fov < best_fov)
		{
			best_fov = fov;
			target = e;
		}
	}

	auto angle = 180.0f;

	if (manual_side == SIDE_LEFT)
		angle = 90.0f;
	else if (manual_side == SIDE_RIGHT)
		angle = -90.0f;

	if (!target)
		return g_ctx.get_command()->m_viewangles.y + angle;

	return math::calculate_angle(g_ctx.globals.eye_pos, target->GetAbsOrigin()).y + angle;
}

bool antiaim::automatic_direction()
{
	float Right, Left;
	Vector src3D, dst3D, forward, right, up;
	trace_t tr;
	Ray_t ray_right, ray_left;
	CTraceFilter filter;

	Vector engineViewAngles;
	m_engine()->GetViewAngles(engineViewAngles);
	engineViewAngles.x = 0.0f;

	math::angle_vectors(engineViewAngles, &forward, &right, &up);

	filter.pSkip = g_ctx.local();
	src3D = g_ctx.globals.eye_pos;
	dst3D = src3D + forward * 100.0f;

	ray_right.Init(src3D + right * 35.0f, dst3D + right * 35.0f);

	g_ctx.globals.autowalling = true;
	m_trace()->TraceRay(ray_right, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	g_ctx.globals.autowalling = false;

	Right = (tr.endpos - tr.startpos).Length();

	ray_left.Init(src3D - right * 35.0f, dst3D - right * 35.0f);

	g_ctx.globals.autowalling = true;
	m_trace()->TraceRay(ray_left, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	g_ctx.globals.autowalling = false;

	Left = (tr.endpos - tr.startpos).Length();

	static auto left_ticks = 0;
	static auto right_ticks = 0;

	if (Left - Right > 10.0f)
		left_ticks++;
	else
		left_ticks = 0;

	if (Right - Left > 10.0f)
		right_ticks++;
	else
		right_ticks = 0;

	if (right_ticks > 10)
		return true;
	else if (left_ticks > 10)
		return false;

	return flip;
}

void antiaim::freestanding(CUserCmd* m_pcmd)
{
	float Right, Left;
	Vector src3D, dst3D, forward, right, up;
	trace_t tr;
	Ray_t ray_right, ray_left;
	CTraceFilter filter;

	Vector engineViewAngles;
	m_engine()->GetViewAngles(engineViewAngles);
	engineViewAngles.x = 0.0f;

	math::angle_vectors(engineViewAngles, &forward, &right, &up);

	filter.pSkip = g_ctx.local();
	src3D = g_ctx.globals.eye_pos;
	dst3D = src3D + forward * 100.0f;

	ray_right.Init(src3D + right * 35.0f, dst3D + right * 35.0f);

	g_ctx.globals.autowalling = true;
	m_trace()->TraceRay(ray_right, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	g_ctx.globals.autowalling = false;

	Right = (tr.endpos - tr.startpos).Length();

	ray_left.Init(src3D - right * 35.0f, dst3D - right * 35.0f);

	g_ctx.globals.autowalling = true;
	m_trace()->TraceRay(ray_left, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	g_ctx.globals.autowalling = false;

	Left = (tr.endpos - tr.startpos).Length();

	static auto left_ticks = 0;
	static auto right_ticks = 0;
	static auto back_ticks = 0;

	if (Right - Left > 20.0f)
		left_ticks++;
	else
		left_ticks = 0;

	if (Left - Right > 20.0f)
		right_ticks++;
	else
		right_ticks = 0;

	if (fabs(Right - Left) <= 20.0f)
		back_ticks++;
	else
		back_ticks = 0;

	if (right_ticks > 10)
		final_manual_side = SIDE_RIGHT;
	else if (left_ticks > 10)
		final_manual_side = SIDE_LEFT;
	else if (back_ticks > 10)
		final_manual_side = SIDE_BACK;
}