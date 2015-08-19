/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  The context handling the gameplay in Neverwinter Nights 2.
 */

#include <cassert>

#include "src/common/error.h"

#include "src/events/events.h"

#include "src/engines/nwn2/game.h"
#include "src/engines/nwn2/nwn2.h"
#include "src/engines/nwn2/console.h"
#include "src/engines/nwn2/campaign.h"
#include "src/engines/nwn2/module.h"
#include "src/engines/nwn2/area.h"

namespace Engines {

namespace NWN2 {

Game::Game(NWN2Engine &engine, ::Engines::Console &console) :
	_engine(&engine), _campaign(0), _console(&console) {

}

Game::~Game() {
	delete _campaign;
}

Campaign &Game::getCampaign() {
	assert(_campaign);

	return *_campaign;
}

Module &Game::getModule() {
	assert(_campaign && _campaign->getModule());

	return *_campaign->getModule();
}

void Game::run() {
	_campaign = new Campaign(*_console);

	while (!EventMan.quitRequested()) {
		runCampaign();
	}

	delete _campaign;
	_campaign = 0;
}

void Game::runCampaign() {
	const std::list<CampaignDescription> &campaigns = _campaign->getCampaigns();
	if (campaigns.empty())
		throw Common::Exception("No campaigns found");

	_campaign->load(*campaigns.begin());

	_campaign->run();

	_campaign->clear();
}

void Game::playMusic(const Common::UString &music) {
	if (!_campaign || !_campaign->getModule() || !_campaign->getModule()->isRunning())
		return;

	Area *area = _campaign->getModule()->getCurrentArea();
	if (!area)
		return;

	area->playAmbientMusic(music);
}

void Game::stopMusic() {
	if (!_campaign || !_campaign->getModule() || !_campaign->getModule()->isRunning())
		return;

	Area *area = _campaign->getModule()->getCurrentArea();
	if (!area)
		return;

	area->stopAmbientMusic();
}

} // End of namespace NWN2

} // End of namespace Engines
