#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

enum
{
	MAIN,
	NONE,
	FIRST,
	SECOND,
	THIRD
};

enum get_side_move
{
	LEFT,
	RIGHT,
	NO_SIDE
};

enum modes
{
	NO_MODE
};

enum resolver_type
{
	ORIGINAL,
	ANIMATION,
	HIGH_DELTA
};

enum resolver_side
{
	RESOLVER_ORIGINAL,
	RESOLVER_ZERO,
	RESOLVER_FIRST,
	RESOLVER_SECOND,
	RESOLVER_LOW_FIRST,
	RESOLVER_LOW_SECOND,
	RESOLVER_HIGH_FIRST,
	RESOLVER_HIGH_SECOND
};

struct matrixes
{
	matrix3x4_t main[MAXSTUDIOBONES];
	matrix3x4_t zero[MAXSTUDIOBONES];
	matrix3x4_t first[MAXSTUDIOBONES];
	matrix3x4_t second[MAXSTUDIOBONES];
	matrix3x4_t third[MAXSTUDIOBONES];
};

class adjust_data;
class resolver_freestand;

class resolver
{
	player_t* player = nullptr;

	adjust_data* player_record = nullptr;
	adjust_data* previous_player_record = nullptr;

	bool side = false;
	bool fake = true;
	bool was_first_bruteforce = false;
	bool was_second_bruteforce = false;

	float lock_side = 0.0f;
	float original_goal_feet_yaw = 0.0f;
	float original_pitch = 89.0f;
public:
	void initialize(player_t* e, adjust_data* record, adjust_data* previous_record, const float& goal_feet_yaw, const float& pitch);
	void reset();
	void animation_resolver(player_t* player, adjust_data* record, adjust_data* previous_record);
	void yaw_bruteforce();
	void get_freestand_data(resolver_freestand& data);
	void run_antifreestand();
	void apply_antifreestand();
	float resolve_pitch();

	AnimationLayer resolver_layers[4][13];
	AnimationLayer previous_layers[13];

	float zero_feet_yaw = 0.0f;
	float resolved_feet_yaw = 0.0f;

	float abs_yaw = 0.0f;

	int bruteforce_ticks = 0;
	int freestand_side = 0;

	resolver_side last_side = RESOLVER_ORIGINAL;
};

class resolver_freestand
{
public:
	bool visible_head = false;
	bool visible_body = false;
	bool visible_leg = false;

	bool data_available = false;
	bool can_use_anti_freestand = false;
};

class resolver_history
{
public:
	float last_resolved_angle = 0.0f;
	resolver_side last_resolved_side = RESOLVER_ORIGINAL;

	int missed_shots = 0;

	bool should_bruteforce = false;

	bool history_available = false;
	bool missed_shot_in_history_side = false;
};

class adjust_data //-V730
{
public:
	player_t* player;
	int i;

	AnimationLayer layers[13];
	AnimationLayer resolver_layers[4][13];

	matrixes matrixes_data;

	resolver_type type;
	resolver_side side;

	bool invalid;
	bool immune;
	bool dormant;
	bool bot;

	bool moving_resolver_active;
	bool high_desync_resolver_active;

	int flags;
	int bone_count;

	float simulation_time;
	float duck_amount;
	float lby;

	Vector angles;
	Vector abs_angles;
	Vector velocity;
	Vector origin;
	Vector mins;
	Vector maxs;

	adjust_data() //-V730
	{
		reset();
	}

	void reset()
	{
		player = nullptr;
		i = -1;

		type = ORIGINAL;
		side = RESOLVER_ORIGINAL;

		invalid = false;
		immune = false;
		dormant = false;
		bot = false;

		moving_resolver_active = false;
		high_desync_resolver_active = false;

		flags = 0;
		bone_count = 0;

		simulation_time = 0.0f;
		duck_amount = 0.0f;
		lby = 0.0f;

		angles.Zero();
		abs_angles.Zero();
		velocity.Zero();
		origin.Zero();
		mins.Zero();
		maxs.Zero();
	}

	adjust_data(player_t* e, bool store = true)
	{
		type = ORIGINAL;
		side = RESOLVER_ORIGINAL;

		invalid = false;
		store_data(e, store);
	}

	void store_data(player_t* e, bool store = true)
	{
		if (!e->is_alive())
			return;

		player = e;
		i = player->EntIndex();

		if (store)
		{
			memcpy(layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
			memcpy(matrixes_data.main, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));
		}

		immune = player->m_bGunGameImmunity() || player->m_fFlags() & FL_FROZEN;
		dormant = player->IsDormant();

#if RELEASE
		player_info_t player_info;
		m_engine()->GetPlayerInfo(i, &player_info);

		bot = player_info.fakeplayer;
#else
		bot = false;
#endif

		flags = player->m_fFlags();
		bone_count = player->m_CachedBoneData().Count();

		simulation_time = player->m_flSimulationTime();
		duck_amount = player->m_flDuckAmount();
		lby = player->m_flLowerBodyYawTarget();

		angles = player->m_angEyeAngles();
		abs_angles = player->GetAbsAngles();
		velocity = player->m_vecVelocity();
		origin = player->m_vecOrigin();
		mins = player->GetCollideable()->OBBMins();
		maxs = player->GetCollideable()->OBBMaxs();
	}

	void adjust_player()
	{
		if (!valid(false))
			return;

		memcpy(player->get_animlayers(), layers, player->animlayer_count() * sizeof(AnimationLayer));
		memcpy(player->m_CachedBoneData().Base(), matrixes_data.main, player->m_CachedBoneData().Count() * sizeof(matrix3x4_t)); //-V807

		player->m_fFlags() = flags;
		player->m_CachedBoneData().m_Size = bone_count;

		player->m_flSimulationTime() = simulation_time;
		player->m_flDuckAmount() = duck_amount;
		player->m_flLowerBodyYawTarget() = lby;

		player->m_angEyeAngles() = angles;
		player->set_abs_angles(abs_angles);
		player->m_vecVelocity() = velocity;
		player->m_vecOrigin() = origin;
		player->set_abs_origin(origin);
		player->GetCollideable()->OBBMins() = mins;
		player->GetCollideable()->OBBMaxs() = maxs;
	}

	bool valid(bool extra_checks = true)
	{
		if (!this) //-V704
			return false;

		if (i > 0)
			player = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!player)
			return false;

		if (player->m_lifeState() != LIFE_ALIVE)
			return false;

		if (immune)
			return false;

		if (dormant)
			return false;

		if (!extra_checks)
			return true;

		if (invalid)
			return false;

		auto net_channel_info = m_engine()->GetNetChannelInfo();

		if (!net_channel_info)
			return false;

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		auto outgoing = net_channel_info->GetLatency(FLOW_OUTGOING);
		auto incoming = net_channel_info->GetLatency(FLOW_INCOMING);

		auto correct = math::clamp(outgoing + incoming + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());

		auto curtime = g_ctx.local()->is_alive() ? TICKS_TO_TIME(g_ctx.globals.fixed_tickbase) : m_globals()->m_curtime; //-V807
		auto delta_time = correct - (curtime - simulation_time);

		if (fabs(delta_time) > 0.2f)
			return false;

		auto extra_choke = 0;

		if (g_ctx.globals.fakeducking)
			extra_choke = 14 - m_clientstate()->iChokedCommands;

		auto server_tickcount = extra_choke + m_globals()->m_tickcount + TIME_TO_TICKS(outgoing + incoming);
		auto dead_time = (int)(TICKS_TO_TIME(server_tickcount) - sv_maxunlag->GetFloat());

		if (simulation_time < (float)dead_time)
			return false;

		return true;
	}
};

class optimized_adjust_data
{
public:
	int i;

	player_t* player;

	float simulation_time;
	float speed;

	bool shot;

	optimized_adjust_data()
	{
		reset();
	}

	void reset()
	{
		i = 0;

		player = nullptr;

		simulation_time = 0.0f;
		speed = 0.0f;

		shot = false;
	}
};

extern std::deque <adjust_data> player_records[65];

class lagcompensation : public singleton <lagcompensation>
{
public:
	void fsn(ClientFrameStage_t stage);
	bool valid(int i, player_t* e);
	void update_player_animations(player_t* e);

	resolver player_resolver[65];
	resolver_history player_history[65];
	resolver_freestand player_freestand[65];

	bool is_dormant[65];

	float previous_goal_feet_yaw[65];
	float feet_delta[65];
};