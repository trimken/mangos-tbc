/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "UpdateData.h"
#include "Player.h"
#include "World.h"

void WorldSession::HandleDuelAcceptedOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    recvPacket >> guid;

    if (!GetPlayer()->duel)                                 // ignore accept from duel-sender
        return;

    Player* pl       = GetPlayer();
    Player* plTarget = pl->duel->opponent;

    if (pl == pl->duel->initiator || !plTarget || pl == plTarget || pl->duel->startTime != 0 || plTarget->duel->startTime != 0)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "WORLD: received CMSG_DUEL_ACCEPTED");
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "Player 1 is: %u (%s)", pl->GetGUIDLow(), pl->GetName());
    DEBUG_FILTER_LOG(LOG_FILTER_COMBAT, "Player 2 is: %u (%s)", plTarget->GetGUIDLow(), plTarget->GetName());

    time_t now = time(nullptr);
    pl->duel->startTimer = now;
    plTarget->duel->startTimer = now;

    if (sWorld.getConfig(CONFIG_BOOL_DUEL_MOD))
    {
		
		pl->SetHealth(pl->GetMaxHealth());
        if (pl->GetPowerType() == POWER_MANA)
            pl->SetPower(POWER_MANA, pl->GetMaxPower(POWER_MANA));

        plTarget->SetHealth(plTarget->GetMaxHealth());
        if (plTarget->GetPowerType() == POWER_MANA)
            plTarget->SetPower(POWER_MANA, plTarget->GetMaxPower(POWER_MANA));
		
         // set max hp, mana and remove buffs of players' pet if they have
        if (Pet* pet = pl->GetPet())
        {
            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(pet->GetPowerType(), pet->GetMaxPower(pet->GetPowerType()));
        }

        if (Pet* petTarget = plTarget->GetPet())
        {
            petTarget->SetHealth(petTarget->GetMaxHealth());
            petTarget->SetPower(petTarget->GetPowerType(), petTarget->GetMaxPower(petTarget->GetPowerType()));
        }

        if (sWorld.getConfig(CONFIG_BOOL_DUEL_CD_RESET) && !pl->GetMap()->IsDungeon())
            pl->RemoveArenaSpellCooldowns();

        if (sWorld.getConfig(CONFIG_BOOL_DUEL_CD_RESET) && !plTarget->GetMap()->IsDungeon())
            plTarget->RemoveArenaSpellCooldowns();
    }
    pl->SendDuelCountdown(3000);
    plTarget->SendDuelCountdown(3000);
}

void WorldSession::HandleDuelCancelledOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_DUEL_CANCELLED");

    // no duel requested
    if (!GetPlayer()->duel)
        return;

    // player surrendered in a duel using /forfeit
    if (GetPlayer()->duel->startTime != 0)
    {
        GetPlayer()->CombatStopWithPets(true);
        if (GetPlayer()->duel->opponent)
            GetPlayer()->duel->opponent->CombatStopWithPets(true);

        GetPlayer()->CastSpell(GetPlayer(), 7267, true);    // beg
        GetPlayer()->DuelComplete(DUEL_WON);
        return;
    }

    // player either discarded the duel using the "discard button"
    // or used "/forfeit" before countdown reached 0
    ObjectGuid guid;
    recvPacket >> guid;

    GetPlayer()->DuelComplete(DUEL_INTERRUPTED);
}
