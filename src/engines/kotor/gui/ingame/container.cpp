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
 *  The ingame container inventory menu.
 */

#include "src/engines/kotor/item.h"

#include "src/engines/aurora/kotorjadegui/panel.h"
#include "src/engines/aurora/kotorjadegui/scrollbar.h"
#include "src/engines/aurora/kotorjadegui/listbox.h"

#include "src/engines/kotor/gui/ingame/container.h"

namespace Engines {

namespace KotOR {

ContainerMenu::ContainerMenu(Console *console) : GUI(console) {
	load("container");

	WidgetPanel *guiPanel = getPanel("TGuiPanel");
	guiPanel->setPosition(-guiPanel->getWidth()/2, -guiPanel->getHeight()/2, 0);

	WidgetListBox *lbItems = getListBox("LB_ITEMS");
	lbItems->setItemType(kLBItemTypeKotORInventory);
	lbItems->setPadding(18);
	lbItems->createItemWidgets(3);
}

void ContainerMenu::fillFromInventory(const Inventory &inv) {
	WidgetListBox *lbItems = getListBox("LB_ITEMS");
	lbItems->removeAllItems();

	const std::map<Common::UString, InventoryItem> &invItems = inv.getItems();
	for (std::map<Common::UString, InventoryItem>::const_iterator i = invItems.begin();
			i != invItems.end(); ++i) {
		try {
			Item item(i->second.tag);
			lbItems->addItem(Common::UString::format("%s|%s|%u",
			                                         item.getName().c_str(),
			                                         item.getIcon().c_str(),
			                                         i->second.count));
		} catch (Common::Exception &e) {
			e.add("Failed to load item %s", i->second.tag.c_str());
			Common::printException(e, "WARNING: ");
		}
	}

	lbItems->refreshItemWidgets();
}

void ContainerMenu::callbackActive(Widget &widget) {
	const Common::UString &tag = widget.getTag();

	if (tag == "BTN_OK") {
		_returnCode = 1;
		return;
	}

	if (tag == "BTN_CANCEL") {
		_returnCode = kReturnCodeAbort;
		return;
	}
}

void ContainerMenu::callbackKeyInput(const Events::Key &key, const Events::EventType &type) {
	if (type == Events::kEventKeyDown) {
		switch (key) {
			case Events::kKeyUp:
				getListBox("LB_ITEMS")->selectPreviousItem();
				break;
			case Events::kKeyDown:
				getListBox("LB_ITEMS")->selectNextItem();
				break;
			default:
				break;
		}
	}
}


} // End of namespace KotOR

} // End of namespace Engines
