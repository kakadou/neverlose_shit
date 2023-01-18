// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "spammers.h"

void spammers::clan_tag()
{
	auto apply = [](const char* tag) -> void
	{
		using Fn = int(__fastcall*)(const char*, const char*);
		static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("engine.dll"), crypt_str("53 56 57 8B DA 8B F9 FF 15")));

		fn(tag, tag);
	};

	static auto removed = false;

	if (!g_cfg.misc.clantag_spammer && !removed)
	{
		removed = true;
		apply(crypt_str(""));
		return;
	}

	if (g_cfg.misc.clantag_spammer)
	{
		auto nci = m_engine()->GetNetChannelInfo();

		if (!nci)
			return;

		static auto time = -1;

		auto main_time = (int)(m_globals()->m_curtime * 1.4) % 4;

		if (main_time != time && !m_clientstate()->iChokedCommands)
		{
			auto tag = crypt_str("");

            /*
			switch (main_time)
			{
            case 0:
                tag = crypt_str(" "); //>V1037
                break;
            case 1:
                tag = crypt_str("|"); //>V1037
                break;
            case 2:
                tag = crypt_str("|\\"); //>V1037
                break;
            case 3:
                tag = crypt_str("|\\|"); //>V1037
                break;
            case 4:
                tag = crypt_str("N"); //>V1037
                break;
            case 5:
                tag = crypt_str("N3"); //>V1037
                break;
            case 6:
                tag = crypt_str("Ne"); //>V1037
                break;
            case 7:
                tag = crypt_str("Ne\\"); //>V1037
                break;
            case 8:
                tag = crypt_str("Ne\\/"); //>V1037
                break;
            case 9:
                tag = crypt_str("Nev"); //>V1037
                break;
            case 10:
                tag = crypt_str("Nev3"); //>V1037
                break;
            case 11:
                tag = crypt_str("Neve"); //>V1037
                break;
            case 12:
                tag = crypt_str("Neve|"); //>V1037
                break;
            case 13:
                tag = crypt_str("Neve|2"); //>V1037
                break;
            case 14:
                tag = crypt_str("Never"); //>V1037
                break;
            case 15:
                tag = crypt_str("Never|"); //>V1037
                break;
            case 16:
                tag = crypt_str("Never|_"); //>V1037
                break;
            case 17:
                tag = crypt_str("Neverl"); //>V1037
                break;
            case 18:
                tag = crypt_str("Neverl0"); //>V1037
                break;
            case 19:
                tag = crypt_str("Neverlo"); //>V1037
                break;
            case 20:
                tag = crypt_str("Neverlo5"); //>V1037
                break;
            case 21:
                tag = crypt_str("Neverlos"); //>V1037
                break;
            case 22:
                tag = crypt_str("Neverlos3"); //>V1037
                break;
            case 23:
                tag = crypt_str("Neverlose"); //>V1037
                break;
            case 24:
                tag = crypt_str("Neverlose."); //>V1037
                break;
            case 25:
                tag = crypt_str("Neverlose.<"); //>V1037
                break;
            case 26:
                tag = crypt_str("Neverlose.c<"); //>V1037
                break;
            case 27:
                tag = crypt_str("Neverlose.cc"); //>V1037
                break;
            case 28:
                tag = crypt_str("Neverlose.cc "); //>V1037
                break;
            case 29:
                tag = crypt_str("Neverlose.cc "); //>V1037
                break;
            case 30:
                tag = crypt_str("Neverlose.cc "); //>V1037
                break;
            case 31:
                tag = crypt_str("Neverlose.cc "); //>V1037
                break;
            case 32:
                tag = crypt_str("Neverlose.cc"); //>V1037
                break;
            case 33:
                tag = crypt_str("Neverlose.c<"); //>V1037
                break;
            case 34:
                tag = crypt_str("Neverlose.<"); //>V1037
                break;
            case 35:
                tag = crypt_str("Neverlose."); //>V1037
                break;
            case 36:
                tag = crypt_str("Neverlose"); //>V1037
                break;
            case 37:
                tag = crypt_str("Neverlos3"); //>V1037
                break;
            case 38:
                tag = crypt_str("Neverlos"); //>V1037
                break;
            case 39:
                tag = crypt_str("Neverlo5"); //>V1037
                break;
            case 40:
                tag = crypt_str("Neverlo"); //>V1037
                break;
            case 41:
                tag = crypt_str("Neverl0"); //>V1037
                break;
            case 42:
                tag = crypt_str("Neverl"); //>V1037
                break;
            case 43:
                tag = crypt_str("Never|_"); //>V1037
                break;
            case 44:
                tag = crypt_str("Never|"); //>V1037
                break;
            case 45:
                tag = crypt_str("Never"); //>V1037
                break;
            case 46:
                tag = crypt_str("Neve|2"); //>V1037
                break;
            case 47:
                tag = crypt_str("Neve|"); //>V1037
                break;
            case 48:
                tag = crypt_str("Neve"); //>V1037
                break;
            case 49:
                tag = crypt_str("Nev3"); //>V1037
                break;
            case 50:
                tag = crypt_str("Nev"); //>V1037
                break;
            case 51:
                tag = crypt_str("Ne\\/"); //>V1037
                break;
            case 52:
                tag = crypt_str("Ne\\"); //>V1037
                break;
            case 53:
                tag = crypt_str("Ne"); //>V1037
                break;
            case 54:
                tag = crypt_str("N3"); //>V1037
                break;
            case 55:
                tag = crypt_str("N"); //>V1037
                break;
            case 56:
                tag = crypt_str("|\\|"); //>V1037
                break;
            case 57:
                tag = crypt_str("|\\"); //>V1037
                break;
            case 58:
                tag = crypt_str("|"); //>V1037
                break;
            case 59:
                tag = crypt_str(" "); //>V1037
                break;
			}
            */

            switch (main_time)
            {
            case 0:
                tag = "nemesis";
                break;
            case 1:
                tag = "n3m3sis";
                break;
            case 2:
                tag = "nemesis";
                break;
            case 3:
                tag = "n3m3sis";
                break;
            }
			apply(tag);
			time = main_time;
		}

		removed = false;
	}
}