#include "dxr.hpp"

DXR::DXR(Table& table) : entries(table.get_sorted_entries()) {
	for(int len=0; len<33; len++){
		for(auto& p: entries[len]){
			uint32_t p_start = p.first;
			uint32_t p_end = p.first | ((2<<(31-len))-1);
			if(len == 32){
				p_end = p_start;
			}

			struct expand_entry p_entry;
			p_entry.start = p_start;
			p_entry.end = p_end;
			p_entry.next_hop = p.second;

			list<struct expand_entry>::iterator it = expansion.begin();
			// Check if there is no entry
			if(it == expansion.end()){
				expansion.insert(it, p_entry);
				continue;
			}
			// Check if there is a hole at the beginning
			else if(it->start > p_end){
				expansion.insert(it, p_entry);
				continue;
			}

			bool mod_in_for = false;
			for(; it != expansion.end(); it++) {
				// Check if the prefix is between this range and the next
				if(next(it) != expansion.end() &&
						it->end < p_start && next(it)->start > p_end){
					expansion.insert(next(it), p_entry);
					mod_in_for = true;
					break;
				}
				// Check if the prefix is at the beginning of the range
				else if(p_start == it->start && p_end < it->end){
					it->start = p_end +1;
					expansion.insert(it, p_entry);
					mod_in_for = true;
					break;
				}
				// Check if the prefix is at the end of the range
				else if(p_start > it->start && p_end == it->end){
					it->end = p_start -1;
					expansion.insert(next(it), p_entry);
					mod_in_for = true;
					break;
				}
				// Check if the prefix is enclosed inside
				// a range in the list
				else if((p_start >= it->start) && (p_end <= it->end)){
					// Set old entry to end before the new entry
					uint32_t old_end = it->end;
					it->end = p.first -1;

					// Insert the new prefix
					expansion.insert(next(it), p_entry);

					// Create the entry to continue the old one
					p_entry.start = p_entry.end +1;
					p_entry.end = old_end;
					p_entry.next_hop = it->next_hop;
					it++; // now points to the new prefix
					expansion.insert(next(it), p_entry);
					mod_in_for = true;
					break;
				}
			}

			// Only continue this iteration if there was no change yet
			if(mod_in_for){
				continue;
			}

			// Check if there is a hole at the end
			if(prev(it)->end < p_start){
				expansion.insert(it, p_entry);
			}
		}
	}
};

uint32_t DXR::route(uint32_t addr) {
	return addr; // Just for editor
};

void DXR::print_expansion(){
	cout << endl << "Expansion table:" << endl;
	for(auto& e : expansion){
		cout << ip_to_str(e.start)
			<< "\t.. " << ip_to_str(e.end)
			<< "\t" << ip_to_str(e.next_hop) << endl;
	}
}
