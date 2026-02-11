#pragma once
struct lz77{
	static const char sp = 124;
	static bool zip(char *out, int &out_len, const int out_cap, const char *in, const int in_len){
		out_len = 0;
		for(int i = 0; i < in_len;){
			int maxx = 0;
			int ans = 0;
			int jj = i - 256;
			if(jj < 0){
				jj = 0;
			}
			for(int j = i - 1; j >= jj; --j){
				for(int k = 0; k < 258; ++k){
					if(in_len <=i + k || in[i + k] - in[j + k]){
						if(maxx < k){
							maxx = k;
							ans = i - j;
						}
						break;
					}
				}
			}
			if(2<maxx){
				if(out_cap <= out_len){assert(0);return false;}out[out_len++] = sp;
				if(out_cap <= out_len){assert(0);return false;}out[out_len++] = maxx - 2;
				if(out_cap <= out_len){assert(0);return false;}out[out_len++] = ans - 1;
				i += maxx;
			}
			else{
				if(out_cap <= out_len){assert(0);return false;}out[out_len++] = in[i];
				if(in[i] == sp){
					if(out_cap <= out_len){assert(0);return false;}out[out_len++] = 0;
				}
				++i;
			}
		}
		return true;
	}
	static bool unzip(char *out, int &out_len, const int out_cap, const char *in, const int in_len){
		out_len = 0;
		for(int i = 0; i < in_len;){
			if(in[i] == sp){
				if(in_len <= ++i){
					assert(0);
					return false;
				}
				if(in[i]){
					int len = (unsigned char)in[i] + 2;
					if(in_len <= ++i){
						assert(0);
						return false;
					}
					int dist = (unsigned char)in[i++] + 1;
					if(out_len < dist){
						assert(0);
						return false;
					}
					if(out_cap < out_len + len){
						assert(0);
						return false;
					}
					for(int j = 0; j < len; ++j){
						out[out_len + j] = out[out_len - dist + j];
					}
					out_len += len;
				}
				else{
					if(out_cap <= out_len){assert(0);return false;}out[out_len++] = sp;
					++i;
				}
			}
			else{
				if(out_cap <= out_len){assert(0);return false;}out[out_len++] = in[i++];
			}
		}
		return true;
	}
};
