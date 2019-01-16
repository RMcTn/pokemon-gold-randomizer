#include "items.h"

Item Items::random_allowed_item() {
	int id = std::rand() % allowed_items.size(); 
	return allowed_items[id];
}

void Items::populate_allowed_items(std::vector<Item> banned_items) {
	for (Item item : items) {
		bool found = false;
		for (Item banned_item : banned_items) {
			if (item.get_id() == banned_item.get_id()) {
				found = true;
				//Skip this item
				break;
			}
		}
		if (!found) {
			allowed_items.push_back(item);
		}

	}
}
