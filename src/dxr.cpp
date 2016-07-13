#include "dxr.hpp"

void DXR::expand(){
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

	// TODO merge
};

void DXR::reduce(){
	vector<list<uint32_t>> pre;
	pre.resize(2<<15);
	for(auto& e : expansion){
		uint16_t upper_start = (e.start & 0xffff0000) >> 16;
		uint16_t upper_end = (e.end & 0xffff0000) >> 16;

		uint16_t lower_start = (e.start & 0x0000ffff);
		//uint16_t lower_end = (e.end & 0x0000ffff); // XXX Not needed?

		uint16_t next_hop_idx = 0xffff;
		// Search for existing entry for the next hop
		for(uint16_t i=0; i<next_hop_table.size(); i++){
			if(next_hop_table[i] == e.next_hop){
				next_hop_idx = i;
			}
		}
		// Insert the next hop if needed
		if(next_hop_idx == 0xffff){
			next_hop_table.push_back(e.next_hop);
			next_hop_idx = next_hop_table.size()-1;
		}

		// Insert the start IPs and next hops in the "pre" buckets
		// upper 16 bit: IP address (lower 16 bit)
		// lower 16 bit: next hop idx
		pre[upper_start].push_back((lower_start << 16) | next_hop_idx);
		for(int i=upper_start+1; i<=upper_end; i++){
			pre[i].push_back(next_hop_idx);
		}
	}

	unsigned int off = 0;
	for(int i=0; i<(2<<15); i++){
		if(pre[i].size() == 1){ // No range table for this entry
			uint32_t entry = 0;
			// format does not make a lot of sense
			// size is 0 -> magic number
			entry |= (*(pre[i].begin()) & 0x0000ffff) << DXR_POSITION_SHIFT;
			lookup_table[i] = entry;
		} else { // We need a range table
			cout << "encountered range, IP: " << ip_to_str(i<<16) << endl;
			unsigned int size = pre[i].size();
			uint32_t entry = 0;
			entry |= DXR_FORMAT;
			entry |= size << DXR_SIZE_SHIFT;
			entry |= off << DXR_POSITION_SHIFT;
			lookup_table[i] = entry;
			off += size;
			for(uint32_t e : pre[i]){
				struct range_long entry;
				entry.start = (e & 0xffff0000) >> 16;
				entry.next_hop = e & 0x0000ffff;
				range_table.push_back(entry);
			}
		}
	}
};

DXR::DXR(Table& table) : entries(table.get_sorted_entries()) {
	expand();
	lookup_table.resize(2<<16);
	reduce();
};

uint32_t DXR::route(uint32_t addr) {
	// TODO Assuming long format for now
	cerr << "IP address: " << ip_to_str(addr) << endl;
	uint32_t lookup_entry = lookup_table[(addr & 0xffff0000) >> 16];
	int pos = (lookup_entry & DXR_POSITION) >> DXR_POSITION_SHIFT;
	int size = (lookup_entry & DXR_SIZE) >> DXR_SIZE_SHIFT;
	if(!size){ // access next_hop_table
		cerr << "Direct lookup in next_hop_table" << endl;
		return next_hop_table[pos];
	}

	uint16_t lower = addr & 0x0000ffff;
	// TODO Should be a binary search instead
	for(int i=pos; i<pos+size; i++){
		uint16_t start = range_table[i].start;
		uint16_t end = range_table[i+1].start;
		if((start < lower) && (lower < end)){
			uint16_t next_hop_idx = range_table[i].next_hop;
			return next_hop_table[next_hop_idx];
		}
	}

	return 0xffffffff;
};

void DXR::print_expansion(){
	cout << endl << "Expansion table:" << endl;
	for(auto& e : expansion){
		cout << ip_to_str(e.start)
			<< "\t.. " << ip_to_str(e.end)
			<< "\t" << ip_to_str(e.next_hop) << endl;
	}
};

void DXR::print_tables(){
	cout << endl << "DXR tables:" << endl;
	for(uint16_t i=0; i<(2<<15)-1; i++){
		uint32_t val = lookup_table[i];
		int val_size = (val & DXR_SIZE) >> DXR_SIZE_SHIFT;
		int val_pos = (val&DXR_POSITION) >> DXR_POSITION_SHIFT;
		if(val_size == 0){
			cout << "Prefix: " << ip_to_str(i<<16) << " next hop: " << val_pos << endl;
		} else {
			cout << "Prefix: " << ip_to_str(i<<16) << endl;
			for(int j=val_pos; j<(val_pos+val_size); j++){
				cout << "    Start: " << ip_to_str(range_table[j].start)
					<< " next hop: " << range_table[j].next_hop << endl;
			}
		}
	}
	for(unsigned int i=0; i<next_hop_table.size(); i++){
		cout << "Next Hop: " << i << " IP: " << ip_to_str(next_hop_table[i]) << endl;
	}
};
