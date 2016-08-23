#include "dxr.hpp"

void DXR::expand(){
	for(int len=32; len>=0; len--){
		list<struct expand_entry>::iterator it1 = expansion.begin();
		list<struct expand_entry>::iterator it2 = next(expansion.begin());

		for(auto& p : entries[len]){
			uint32_t p_start = p.first;
			uint32_t p_end = p.first | ((2<<(31-len))-1);
			if(len == 32){
				p_end = p_start;
			}

			struct expand_entry p_entry;
			p_entry.next_hop = p.second;

			// Check if there is no entry
			if(it1 == expansion.end()){
				p_entry.start = p_start;
				p_entry.end = p_end;

				expansion.insert(it1, p_entry);
				//continue;
				goto sanity; // XXX debug
			}

			// XXX: Sanity check
			if(it1->start > it1->end){
				cerr << "[ERROR] it1->start > it1->end" << endl;
			}
			if((it2 != expansion.end()) && (it1->end > it2->start)){
				cerr << "[ERROR] it1->end > it2->start" << endl;
			}
			if(it1 != prev(it2)){
				cerr << "[ERROR] it1 != prev(it2)" << endl;
			}

			if(it1 == expansion.begin()){
				// Are we at the very beginning?
				if(p_end < it1->start){
					p_entry.start = p_start;
					p_entry.end = p_end;
					expansion.insert(it1, p_entry);
					// Repair iterators
					it1 = expansion.begin();
					it2 = next(it1);
					//continue;
					goto sanity; // XXX debug
				}
				// Same as above, ending inside it1?
				else if(p_start < it1->start &&
						p_end > it1->start && p_end < it1->end){
					p_entry.start = p_start;
					p_entry.end = it1->start-1;
					expansion.insert(it1, p_entry);
					//continue;
					goto sanity; // XXX debug
				}
				// All the space before it1 and maybe beyond?
				else if(p_start < it1->start){
					p_entry.start = p_start;
					p_entry.end = it1->start-1;
					expansion.insert(it1, p_entry);
				}
			}

			while(it2 != expansion.end() && it1->end < p_end){
				bool finished_entry = false;
				// Is there even space between it1 and it2?
				if(it1->end == (it2->start -1)){
					goto checks_done;
				}

				// Do we fill the space between the two "it"s ?
				if(p_start < it1->end && p_end > it2->start){
					p_entry.start = it1->end+1;
					p_entry.end = it2->start-1;

					expansion.insert(it2, p_entry);
				}
				// Are we somewhere in between?
				else if(p_start > it1->end && p_end < it2->start){
					finished_entry = true;

					p_entry.start = p_start;
					p_entry.end = p_end;

					expansion.insert(it2, p_entry);
					it1 = prev(it2);
					break;
				}
				// Are we starting inside it1 and stopping before it2?
				else if(p_start >= it1->start && p_start <= it1->end &&
						p_end < it2->start){
					finished_entry = true;

					p_entry.start = it1->end+1;
					p_entry.end = p_end;

					expansion.insert(it2, p_entry);
					it1 = prev(it2);
					break;
				}
				// Are we starting after it1 and stopping inside it2?
				else if(p_start > it1->end && p_end <= it1->start &&
						p_end <= it2->end){
					finished_entry = true;

					p_entry.start = p_start;
					p_entry.end = it2->start-1;

					expansion.insert(it2, p_entry);
				}

				if(p_entry.start > p_entry.end){
					cerr << "[ERROR] p_entry.start > p_entry.end" << endl;
				}

				checks_done:
				it2++;
				it1 = prev(it2);

				if(finished_entry){
					break;
				}
			}

			// Are we at the very end?
			if(it2 == expansion.end()){
				// Do we start after it1?
				if(it1->end < p_start){
					p_entry.start = p_start;
					p_entry.end = p_end;

					expansion.insert(it2, p_entry);
					it1 = prev(it2);
				}
				// Do we start before it1?
				else if(p_start < it1->end && p_end > it1->end){
					p_entry.start = it1->end+1;
					p_entry.end = p_end;

					expansion.insert(it2, p_entry);
					it1 = prev(it2);
				}
			}

sanity:
			// XXX: Sanity check
			if(it1->start > it1->end){
				cerr << "[ERROR] it1->start > it1->end" << endl;
			}
			if((it2 != expansion.end()) && (it1->end > it2->start)){
				cerr << "[ERROR] it1->end > it2->start" << endl;
			}
			if(p_entry.start > p_entry.end){
				cerr << "[ERROR] p_entry.start > p_entry.end" << endl;
			}
		}

		// XXX debug
		//cout << endl << endl << "expansion after prefix length: " << len << endl;
		//print_expansion();
	}

	// merge entries
	list<struct expand_entry>::iterator it1 = expansion.begin();
	list<struct expand_entry>::iterator it2 = next(expansion.begin());
	while(it2 != expansion.end()){
		if(it1->next_hop == it2->next_hop){
			it1->end = it2->end;
			expansion.erase(it2);
			it2 = next(it1);
		}
		it1++;
		it2++;
	}
}

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
	uint32_t lookup_entry = lookup_table[(addr & 0xffff0000) >> 16];
	int pos = (lookup_entry & DXR_POSITION) >> DXR_POSITION_SHIFT;
	int size = (lookup_entry & DXR_SIZE) >> DXR_SIZE_SHIFT;
	if(!size){ // access next_hop_table
		return next_hop_table[pos];
	}

	uint16_t lower = addr & 0x0000ffff;
	// TODO Should be a binary search instead
	for(int i=pos; i<pos+size-1; i++){
		uint16_t start = range_table[i].start;
		uint16_t end = range_table[i+1].start;
		if((start <= lower) && (lower < end)){
			uint16_t next_hop_idx = range_table[i].next_hop;
			return next_hop_table[next_hop_idx];
		}
	}

	return next_hop_table[range_table[pos+size].next_hop];

//	return 0xffffffff;
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
