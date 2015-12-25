#include "shardBuffer.hpp"

int shard_buffer_t::set(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  entries[idx] = make_pair(0,0);
  growbuf[idx] = string(userbuf, userbuf_sz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::get(size_t idx, char* userbuf, size_t &userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_loc = entries[idx].first;
  size_t chunk_sz  = entries[idx].second;
  size_t data_sz   = chunk_sz + growbuf[idx].size();
  if (data_sz > userbuf_sz)
    return _SHARDBUF_OVERFLOW;

  userbuf_sz = 0;
  if (chunk_sz > 0) {
    size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log); 
    size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);
    memcpy(userbuf, &(chunks[chunk_idx][chunk_offset]), chunk_sz);
    userbuf_sz += chunk_sz;
  }

  if (growbuf[idx].size() > 0) {
    memcpy(userbuf+userbuf_sz, growbuf[idx].data(), growbuf[idx].size());
    userbuf_sz += growbuf[idx].size();
  }
  return _SHARDBUF_SUCCEED; 
}

int shard_buffer_t::app(size_t idx, const char* userbuf, size_t userbuf_sz) {
  CHECK_OUT_RANGE(idx);
  growbuf[idx].append(userbuf, userbuf_sz);
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx) {
  CHECK_OUT_RANGE(idx);
  entries[idx].first = 0;
  entries[idx].second = 0;
  growbuf[idx].clear();
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::del(size_t idx, size_t offset, size_t sz) {
  CHECK_OUT_RANGE(idx);
  size_t chunk_loc = entries[idx].first;
  size_t chunk_sz  = entries[idx].second;

  size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log); 
  size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);

  if (offset+sz <= chunk_sz) { // delete within the chunk
    chunks[chunk_idx].erase(chunk_offset+offset, sz);
    entries[idx] = make_pair(chunk_loc, chunk_sz-sz);
  } else if (offset <= chunk_sz) { // delete in both
    size_t delta = offset+sz - chunk_sz;
    chunks[chunk_idx].erase(chunk_offset+offset, sz-delta);
    growbuf[idx].erase(0, delta);
  } else if (offset+sz-chunk_sz < growbuf[idx].size()) { // delete in growbuf only
    offset -= chunk_sz;
    growbuf[idx].erase(offset, sz);
  }
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::push_back(const char* buf, size_t bufsz) {
  entries.push_back(make_pair(0,0));
  growbuf.push_back(string(buf, bufsz));
  return _SHARDBUF_SUCCEED;
}

int shard_buffer_t::serialize() {
  size_t num_new_chunks = 0;
  for (auto it=entries.begin(); it!=entries.end(); it++) 
    num_new_chunks = (num_new_chunks>it->first)? num_new_chunks: it->first;
  num_new_chunks >>= chunk_log;

  vector<string> new_chunks(num_new_chunks);
  for (size_t i=0; i<entries.size(); i++) {
    size_t chunk_loc = entries[i].first;
    size_t chunk_sz  = entries[i].second;
   
    size_t chunk_idx    = GET_CHUNK_INDEX(chunk_loc, chunk_log); 
    size_t chunk_offset = GET_CHUNK_OFFSET(chunk_loc, chunk_log);

    size_t new_chunk_offset = new_chunks[chunk_idx].size();
    size_t new_chunk_datasz = chunk_sz + growbuf[i].size();
    entries[i] = make_pair(new_chunk_offset, new_chunk_datasz);

    new_chunks[chunk_idx].append(&chunks[chunk_idx][chunk_offset], chunk_sz);
    new_chunks[chunk_idx].append(growbuf[i]);    
  }
  chunks = move(new_chunks);
  growbuf.clear();
}

void shard_buffer_t::dump() {
  for (size_t i=0; i<entries.size(); i++) {
    cout << "entry (" << entries[i].first <<"," 
	 << entries[i].second << "): [" 
	 << growbuf[i] << "]\n";
  }
  
  for (size_t i=0; i<chunks.size(); i++) {
    cout << "chunk-" << i << ": ["
	 << chunks[i] << "]\n";
  }
}


#ifdef DEBUG
int main() {
  shard_buffer_t buf;

  /*
  string str  = "hello ";
  string str2 = "world!";
  buf.push_back(str.c_str(), str.size());
  // buf.push_back(str2.c_str(), str2.size());
  buf.app(0, str2.c_str(), str2.size());

  char mybuf[1024];
  size_t sz = 1024;
  if (!buf.get(0, mybuf, sz))
    cout << "--> " << string(mybuf, sz) << endl;
  */


  ifstream f("sample.txt");
  if (!f.is_open()) {
    cerr << "file can not open" << endl;
    return -1;
  }

  while (f.good()) {
    string line;
    getline(f, line);

    buf.push_back(str.c_str(), str.size());
  }
  f.close();

  buf.dump();


  return 0;
}
#endif
