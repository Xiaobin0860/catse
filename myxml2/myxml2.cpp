#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#define assert(exp) (exp) || (printf("#exp=%s, __FILE__=%s, __LINE__=%d\n", #exp, __FILE__, __LINE__), getchar(), exit(0), 0)
struct tinfTree{
	short table[16];
	short trans[288];
};
struct tinfData{
	char*source;
	int bitcount;
	char*dest;
	struct tinfTree ltree;
	struct tinfTree dtree;
};
static void tinfBuildTree(struct tinfTree*t, char*lengths, int num){
	memset(t->table, 0, sizeof(t->table));
	int i = 0;
	for(i = 0; i < num; ++i){
		++t->table[lengths[i]];
	}
	t->table[0] = 0;
	int sum = 0;
	short offs[16];
	for(i = 0; i < 16; ++i){
		offs[i] = sum;
		sum += t->table[i];
	}
	for(i = 0; i < num; ++i){
		if(lengths[i]){
			t->trans[offs[lengths[i]]++] = i;
		}
	}
}
static int tinfReadBits(struct tinfData*d, int num){
	if(num == 0){
		return 0;
	}
	assert(0 < num && num < 14);
	if(!d->bitcount){
		d->source++;
	}
	int sum = 0;
	int weight = 0;
	for(;;){
		if(d->bitcount + num < 9){
			sum |= (*(d->source - 1) >> d->bitcount & (1 << num) - 1) << weight;
			d->bitcount += num;
			d->bitcount &= 7;
			return sum;
		}
		sum |= (*(d->source - 1) & 255) >> d->bitcount << weight;
		weight += 8 - d->bitcount;
		num -= 8 - d->bitcount;
		d->bitcount = 0;
		++d->source;
	}
}
static int tinfDecodeSymbol(struct tinfData*d, struct tinfTree*t){
	int sum = 0, cur = 0, len = 0;
	do{
		cur <<= 1;
		cur |= tinfReadBits(d, 1);
		++len;
		sum += t->table[len];
		cur -= t->table[len];
	}while(0 <= cur);
	return t->trans[sum + cur];
}
static void tinfInflateBlockData(struct tinfData*d, struct tinfTree*lt, struct tinfTree*dt){
	for(;;){
		int sym = tinfDecodeSymbol(d, lt);
		if (sym == 256){
			return;
		}
		if (sym < 256){
			*d->dest++ = sym;
		}
		else{
			sym -= 257;
			static char lengthBits[]  = {0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2,  2,  2,  3,  3,  3,  3,   4,   4,   4,   4,   5,   5,   5,   5,     0,    6};
			static short lengthBase[] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23, 27, 31, 35, 43, 51, 59,  67,  83,  99, 115, 131, 163, 195, 227,   258,  323};
			int length = tinfReadBits(d, lengthBits[sym]) + lengthBase[sym];
			int dist = tinfDecodeSymbol(d, dt);
			static char distBits[]    = {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,  12,    13,   13};
			static short distBase[]   = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
			int offs = tinfReadBits(d, distBits[dist]) + distBase[dist];
			int i;
			for(i = 0; i < length; ++i){
				d->dest[i] = d->dest[i - offs];
			}
			d->dest += length;
		}
	}
}
static void tinfUncompress(void*dest, int*destLen, void*source, int sourceLen){
	struct tinfData d;
	d.source = (char*)source;
	d.bitcount = 0;
	d.dest = (char*)dest;
	int bfinal;
	do{
		bfinal = tinfReadBits(&d, 1);
		int btype = tinfReadBits(&d, 2);
		switch(btype){
			case 0:	{
					int length = *(short*)d.source & 65535;
					if(length - (~*(short*)(d.source + 2) & 65535)){
						assert(0);
					}
					d.source += 4;
					memcpy(d.dest, d.source, length);
					d.dest += length;
					d.source += length;
					d.bitcount = 0;
				}
				break;
			case 1:	{
					static struct tinfTree sltree = {{0,0,0,0,0,0,0,24,152,112}, {256,257,258,259,260,261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,280,281,282,283,284,285,286,287,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}};
					static struct tinfTree sdtree = {{0,0,0,0,0,32}, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31}};
					tinfInflateBlockData(&d, &sltree, &sdtree);
				}
				break;
			case 2:{
				       int hlit = tinfReadBits(&d, 5) + 257;
				       int hdist = tinfReadBits(&d, 5) + 1;
				       int hclen = tinfReadBits(&d, 4) + 4;
				       int i;
				       static char clcidx[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
				       char lengthsA[sizeof(clcidx)] = {};
				       for(i = 0; i < hclen; ++i){
					       lengthsA[clcidx[i]] = tinfReadBits(&d, 3);
				       }
				       struct tinfTree codeTree;
				       tinfBuildTree(&codeTree, lengthsA, sizeof(lengthsA));
				       int num;
				       char lengthsB[320] = {};
				       int len = hlit + hdist;
				       for(num = 0; num < len;){
					       int sym = tinfDecodeSymbol(&d, &codeTree);
					       switch (sym){
						       int length;
						       case 16:
						       length = tinfReadBits(&d, 2) + 3;
						       memset(lengthsB + num, lengthsB[num - 1], length);
						       num += length;
						       break;
						       case 17:
						       num += tinfReadBits(&d, 3) + 3;
						       break;
						       case 18:
						       num += tinfReadBits(&d, 7) + 11;
						       break;
						       default:
						       lengthsB[num++] = sym;
						       break;
					       }
				       }
				       tinfBuildTree(&d.ltree, lengthsB, hlit);
				       tinfBuildTree(&d.dtree, lengthsB + hlit, hdist);
			       }
			       tinfInflateBlockData(&d, &d.ltree, &d.dtree);
			       break;
			default:
			       assert(0);
		}
	}while(!bfinal);
	*destLen = d.dest - (char*)dest;
}
struct kv{
	char*k;
	char*v;
};
struct dot{
	char*name;
	char*text;
	struct kv*vkv[2];
};
struct tri{
	char filename[64];
	short compressionMethod;
	int compressedSize;
	int uncompressedSize;
	int offset;
};
struct shareCell{
	char*cell;
	int cnt;
};
static int cmpCell(const void*a, const void*b){
	return ((struct shareCell*)a)->cell - ((struct shareCell*)b)->cell;
}
struct shareRow{
	int cnt;
	int m;	
	char**varType;
	char**name;
	char**row;
};
static int cmpRow(const void*aa, const void*bb){
	struct shareRow*a = (struct shareRow*)aa;
	struct shareRow*b = (struct shareRow*)bb;
	int m = a->m;
	int ret = m - b->m;
	if(ret){
		return ret;
	}
	int j;
	char**aName = a->name;
	char**bName = b->name;
	for(j = 1; j < m; ++j){
		ret = aName[j] - bName[j];
		if(ret){
			return ret;
		}
	}
	char**aRow = a->row;
	char**bRow = b->row;
	for(j = 1; j < m; ++j){
		ret = aRow[j] - bRow[j];
		if(ret){
			return ret;
		}
	}
	return 0;
}
struct shareSheet{
	int cnt;
	int n;
	int m;
	char***sheet;
};
static int cmpSheet(const void*aa, const void*bb){
	struct shareSheet*a = (struct shareSheet*)aa;
	struct shareSheet*b = (struct shareSheet*)bb;
	int n = a->n;
	int ret = n - b->n;
	if(ret){
		return ret;
	}
	int m = a->m;
	ret = m - b->m;
	if(ret){
		return ret;
	}
	int i;
	for(i = 0; i < n; ++i){
		char**aa = a->sheet[i];
		char**bb = b->sheet[i];
		int j;
		for(j = 0; j < m; ++j){
			int ret = aa[j] - bb[j];
			if(ret){
				return ret;
			}
		}
	}
	return 0;
}
static void fileStr(char*out, int outCap, char*filename, FILE *file, struct tri *files, int filesLen){ 
	int i;
	for(i = 0; i < filesLen; ++i){
		if(strcmp(files[i].filename, filename)){
			continue;
		}
		if(fseek(file, files[i].offset, SEEK_SET)){
			assert(0);
		}
		if(outCap <= files[i].uncompressedSize){
			assert(0);
		}
		if(files[i].compressionMethod == 0x8){
			static char bufCompress[1 << 22];
			if(sizeof(bufCompress) < files[i].compressedSize){
				assert(0);
			}
			if(1 - fread(bufCompress, files[i].compressedSize, 1, file)){
				assert(0);
			}
			int bufUncompressLen = 0;
			tinfUncompress(out, &bufUncompressLen, bufCompress, files[i].compressedSize);
			if(bufUncompressLen - files[i].uncompressedSize){
				assert(0);
			}
		}
		else if(files[i].compressionMethod == 0x0){
			if(1 - fread(out, files[i].uncompressedSize, 1, file)){
				assert(0);
			}
		}
		else{
			assert(0);
		}
		out[files[i].uncompressedSize] = 0;
		return;
	}
	assert(0);
}
static struct dot*getXml(char*begin){
#define checkEnd if(end <= begin){assert(0);}
	static char*pname[] = {"si","t",  "sheet",  "c","v"};
	static char*pk[] = {"name", "r", "t"};
	static char hasInit;
	char*end = begin + strlen(begin);
	static struct dot vd[1 << 20];
	int vdLen = 0;
	static struct kv bufVkv[1 << 19];
	int bufVkvLen = 0;
	for(;;){
		++begin;checkEnd;
		if(*begin == '?'){
			while(*begin - '<'){	
				++begin;checkEnd;
			}
			continue;
		}
		if(*begin == '/'){
			while(*begin - '>'){
				++begin;checkEnd;
			}
			++begin;
			if(end<=begin){
				vd[vdLen].name = 0;
				return vd;
			}
			continue;
		}
		char*key = begin;
		while(*begin - 32 && *begin - '>' && (*begin - '/' || *(begin + 1) - '>')){
			++begin;checkEnd;
		}
		char t = *begin;
		*begin = 0;
		char*pp = 0;
		int i;
		for(i = 0; i < sizeof(pname) / sizeof(*pname); ++i){
			if(!strcmp(key, pname[i])){
				pp = pname[i];
				break;
			}
		}
		*begin = t;
		struct dot*back = 0;
		if(pp){
			if(sizeof(vd) / sizeof(*vd) < vdLen + 2){
				assert(0);
			}
			back = vd + vdLen++;
			memset(back, 0, sizeof(*back));
			back->name = pp;
		}
		int vkvLen = 0;
		while(*begin - '>' && (*begin - '/' || *(begin + 1) - '>')){
			++begin;checkEnd;
			char*key = begin;
			while(*begin - '='){
				++begin;checkEnd;
			}
			char*ppp = 0;
			if(pp){
				char t = *begin;
				*begin = 0;
				for(i = 0; i < sizeof(pk) / sizeof(*pk); ++i){
					if(!strcmp(key, pk[i])){
						ppp = pk[i];
						break;
					}
				}
				*begin = t;
				if(ppp){
					if(sizeof(back->vkv) / sizeof(*back->vkv) <= vkvLen){
						assert(0);
					}
					if(sizeof(bufVkv) / sizeof(*bufVkv) <= bufVkvLen){
						assert(0);
					}
					back->vkv[vkvLen++] = bufVkv + bufVkvLen++;
					back->vkv[vkvLen - 1]->k = ppp;
				}
			}
			begin += 2;checkEnd;
			char*old = begin;
			while(*begin - '\"'){
				++begin;checkEnd;
			}
			if(ppp){
				back->vkv[vkvLen - 1]->v = old;
				*begin = 0;
			}
			++begin;checkEnd;
		}
		if(*begin == '>'){
			++begin;checkEnd;
			if(back){
				back->text = begin;
			}
			while(*begin - '<'){
				++begin;checkEnd;
			}
			*begin = 0;
		}
		else if(*begin == '/' && *(begin + 1) == '>'){	
			begin += 2;checkEnd;
			if(back){
				back->text = "";
			}
		}
		else{
			assert(0);
		}
	}
#undef checkEnd
}
static char*getStr(char*str){
	static char *fix[] = {"number","plain","string","table","tbstr"};
	int i;
	for(i = 0; i < sizeof(fix) / sizeof(*fix); ++i){
		if(!strcmp(str, fix[i])){
			return fix[i];
		}
	}
	static char*set[65536];
	static int setlen;
	int a = 0, b = setlen - 1;
	while(a<=b){
		int c = a + b >> 1;
		int t = strcmp(str, set[c]);
		if(t < 0){
			b = c - 1;
		}
		else if(0 < t){
			a = c + 1;
		}
		else{
			return set[c];
		}
	}
	if(sizeof(set)/sizeof(*set)<=setlen){
		assert(0);
	}
	memmove(set + a + 1, set + a, sizeof(*set) * (setlen - a));
	++setlen;
	static char buf[1 << 22];
	static int buflen;
	int len = strlen(str);
	if(sizeof(buf)<=buflen + len){
		assert(0);
	}
	set[a] = buf + buflen;
	memcpy(buf + buflen, str, len);
	buflen += len + 1;
	return set[a];
}
static void getWorkbook(char**out, int outCap, char*workbookBegin){
	struct dot*vd = getXml(workbookBegin);
	int outLen = 0;
	int i;
	for(i = 0; vd[i].name; ++i){
		if(vd[i].name == "sheet"){
			if(outCap <= outLen + 1){
				assert(0);
			}
			out[outLen++] = getStr(vd[i].vkv[0]->v);
		}
	}
	out[outLen] = 0;
}
static void getSharedStrings(char**out, int outCap, char*sharedStringsBegin){
	struct dot*vd = getXml(sharedStringsBegin);
	int outlen = 0;
	int i;
	char buf[65536];
	int buflen;
	for(i = 0; vd[i].name; ++i){
		if(vd[i].name == "si"){
			if(outCap <= outlen){
				assert(0);
			}
			if(outlen){
				buf[buflen] = 0;
				out[outlen - 1] = getStr(buf);
			}
			++outlen;
			buflen = 0;
		}else if(vd[i].name == "t"){
			int textlen = strlen(vd[i].text);
			if(sizeof(buf) <= buflen + textlen){
				assert(0);
			}
			memcpy(buf + buflen, vd[i].text, textlen);
			buflen += textlen;
		}
	}
	if(outlen){
		buf[buflen] = 0;
		out[outlen - 1] = getStr(buf);
	}
}
static void strReplace(char*a, char*b, char*c){
	int alen = strlen(a);
	int blen = strlen(b);
	int clen = strlen(c);
	if(blen < clen){
		assert(0);
	}
	char*pp = a;
	char*p;
	for(p = a; *p;){
		if(p + blen <= a + alen && !memcmp(p, b, blen)){
			memcpy(pp, c, clen);
			pp += clen;
			p += blen;
		}
		else{
			*pp++ = *p++;
		}
	}
	if(*pp){
		*pp = 0;
	}
}
static void removeSpace(char*a){
	char*aOld = a;
	int cntYin = 0;
	int cntKuo = 0;
	char*b = a;
	for(;*a; ++a){
		if(*a == '\"'){
			cntYin = ~cntYin;
		}
		else if(*a == '[' && *(a + 1) == '['){
			++cntKuo;
			if(1 < cntKuo){
				FILE*file = fopen("debug.txt", "wb");
				fwrite(aOld, 1, strlen(aOld), file);
				assert(0);
			}
		}
		else if(*a == ']' && *(a + 1) == ']'){
			--cntKuo;
			if(cntKuo < 0){
				FILE*file = fopen("debug.txt", "wb");
				fwrite(aOld, 1, strlen(aOld), file);
				assert(0);
			}
		}
		if(cntYin || cntKuo || *a - 10 && *a - 13 && *a - 32){
			*b++ = *a;
		}
	}
	if(*b){
		*b = 0;
	}
	a = aOld;
	b = a;
	for(;*a; ++a){
		if(!(*a == ',' && *(a+1) == '}')){
			*b++ = *a;
		}
	}
	if(*b){
		*b = 0;
	}
	if(aOld < b && *(b - 1) == ','){
		*(b - 1) = 0;
	}
}
static void getStrAddQuote(char*out, int outCap, char*a){
	int outLen = 0;
	char*last = a;
	char*start = 0;
	char hasLetter = 0;
	int cntKuo = 0;
	char*aOld = a;
	for(;; ++a){
		if(*a == '('){
			++cntKuo;
		}
		else if(*a == ')'){
			--cntKuo;
		}
		else if(!*a || *a == ',' || *a == '}'){
			if(start){
				if(hasLetter){
					if(!cntKuo){
						int len = start - last;if(outCap < outLen + len){assert(0);}memcpy(out + outLen, last, len);outLen += len;
						if(outCap <= outLen){assert(0);}out[outLen++] = '\"';
						len = a - start;if(outCap < outLen + len){assert(0);}memcpy(out + outLen, start, len);outLen += len;
						if(outCap <= outLen){assert(0);}out[outLen++] = '\"';
						last = a;
						start = 0;
					}
				}
				else{
					start = 0;
				}
			}
			if(!*a){
				break;
			}
		}
		else if(*a == 10 || *a == 13 || *a == 32 || *a == '{'){}
		else{
			if(!start){
				start = a;
				cntKuo = 0;
				hasLetter = 0;
			}
			if(!(*a == '-' || *a == '.' || 47 < *a && *a < 58)){
				hasLetter = 1;
			}
		}
	}
	if(aOld < last){
		int len = a - last;if(outCap < outLen + len){assert(0);}memcpy(out + outLen, last, len);outLen += len;
	}
	if(outCap <= outLen){assert(0);}out[outLen] = 0;
}
static int cmp(const void*a, const void*b){
	return *(char**)a - *(char**)b;
}
static char***getVvs(int*n, int*m, char**vs, char*sheetNameBegin, char*sheetname, char*filename){
	static char*vvs[20000][128];
	memset(vvs, 0, sizeof(vvs));
	struct dot*vd = getXml(sheetNameBegin);
	int ist;
	int col = 0;
	*n = 0;
	*m = 0;
	int i;
	int rowOld = 0;
	for(i = 0; vd[i].name; ++i){
		if(vd[i].name == "c"){
			col = 0;
			char*p;
			for(p = vd[i].vkv[0]->v; 'A'<=*p && *p<='Z'; ++p){
				col *= 26;
				col += *p - 64;
			}
			--col;
			int row = atoi(p);
			if(row < rowOld){
				assert(0);
			}
			if(rowOld < row){
				rowOld = row;
				if(sizeof(vvs) /sizeof(*vvs) <= *n){
					assert(0);
				}
				++*n;
			}
			ist = vd[i].vkv[1] && vd[i].vkv[1]->v[0] == 's' && !vd[i].vkv[1]->v[1];
		}else if(vd[i].name == "v"){
			int textlen = strlen(vd[i].text);
			if(ist){
				int sum = 0;
				int j;
				for(j = 0; j < textlen; ++j){
					sum *= 10;
					sum += vd[i].text[j] - 48;
				}
				vvs[*n - 1][col] = vs[sum];
			}
			else{
				if(textlen){
					vvs[*n - 1][col] = vd[i].text;
				}
			}
			if(*m <= col){
				if(sizeof(*vvs) / sizeof(**vvs) < col + 1){
					assert(0);
				}
				*m = col + 1;
			}
		}
	}
	if(*n < 2){
		assert(0);
	}
	char f[sizeof(*vvs) / sizeof(**vvs)];
	int len = 1;
	for(i = 1; i < *m; ++i){
		f[i] = vvs[0][i] && vvs[0][i][0] && vvs[1][i] && vvs[1][i][0];
		if(f[i]){
			if(len < i){
				vvs[0][len] = vvs[0][i];
				vvs[1][len] = vvs[1][i];
			}
			++len;
		}
	}
	while(*m && !f[*m - 1]){
		--*m;
	}
	int ii = 2;
	int jj = 1;
	for(i = 3; i < *n; ++i){
		if(vvs[i][0] && vvs[i][0][0]){
			jj = 1;
			int j;
			for(j = 1; j < *m; ++j){
				if(f[j]){
					vvs[i][jj++] = vvs[i][j];
				}
			} 
			memcpy(vvs[ii++], vvs[i], sizeof(*vvs));
		}
	}
	*n = ii;
	*m = jj;
	for(i = *n - 1; i >= 0; --i){
		int j;
		for(j = *m - 1; j >= 0; --j){
			if(!vvs[i][j]){
				vvs[i][j] = "";
			}
		}
	}
	for(i = 0; i < 2; ++i){
		int j;
		for(j = 0; j < *m; ++j){
			vvs[i][j] = getStr(vvs[i][j]);
		}
	}
	char**varType = vvs[0];
	for(i = 2; i < *n; ++i){
		char**row = vvs[i];
		row[0] = getStr(row[0]);
		int j;
		for(j = 1; j < *m; ++j){
			char*cell = row[j];
			if(varType[j] == "plain" || varType[j] == "string" || varType[j] == "table" || varType[j] == "tbstr"){
				strReplace(cell, "\r", "");
				strReplace(cell, "&amp;", "&");
				strReplace(cell, "&gt;", ">");
				strReplace(cell, "&quot;", "\"");
				strReplace(cell, "&lt;", "<");
				if(varType[j] == "table" || varType[j] == "tbstr"){
					if(varType[j] == "tbstr"){
						char cellNew[4096];
						getStrAddQuote(cellNew, sizeof(cellNew), cell);
						if(cellNew[0]){
							cell = cellNew;
						}
					}
					removeSpace(cell);
				}
			}
			row[j] = getStr(cell);
		}
	}
	char*vc[sizeof(vvs) / sizeof(*vvs)];
	if(sizeof(vc)/sizeof(*vc)<*n - 2){
		assert(0);
	}
	for(i = 2; i < *n; ++i){
		vc[i - 2] = vvs[i][0]; 
	}
	qsort(vc, *n - 2, sizeof(*vc), cmp);
	for(i = 1; i < *n - 2; ++i){
		if(vc[i - 1] == vc[i]){
			printf("%s表的%s标签页中第一列存在重复的ID[%s]\n", filename, sheetname, vc[i]);
			assert(0);
		}
	}
	if(sizeof(vc)/sizeof(*vc)<*m - 1){
		assert(0);
	}
	for(i = 1; i < *m; ++i){
		vc[i - 1] = vvs[1][i];
	}
	qsort(vc, *m - 1, sizeof(*vc), cmp);
	for(i = 1; i < *m - 1; ++i){
		if(vc[i - 1] == vc[i]){
			printf("%s表的%s标签页中存在重复的列[%s]\n", filename, sheetname, vc[i]);
			assert(0);
		}
	}
	static char vvsMem[1 << 22];
	static int vvsMemLen;
	int siz = *n * sizeof(char*);
	if(sizeof(vvsMem) < vvsMemLen + siz){
		assert(0);
	}
	char***vvsNew = (char***)(vvsMem + vvsMemLen);
	vvsMemLen += siz;
	for(i = 0; i < *n; ++i){
		int siz = *m * sizeof(char*);
		if(sizeof(vvsMem) < vvsMemLen + siz){
			assert(0);
		}
		vvsNew[i] = (char**)(vvsMem + vvsMemLen);
		vvsMemLen += siz;
		memcpy(vvsNew[i], vvs[i], siz);
	}
	return vvsNew;
}
static int writeInt(char*out, int cap, int a){
	if(cap < 16){
		assert(0);
	}
	if(a < 0){
		assert(0);
	}
	int outLen = 0;
	do{
		int t = a;
		a /= 10;
		out[outLen++] = t - a * 10 + 48;
	}while(a);
	int i;
	for(i = (outLen >> 1) - 1; i >= 0; --i){
		int j = outLen - 1 - i;
		out[i] ^= out[j];
		out[j] ^= out[i];
		out[i] ^= out[j];
	}
	return outLen;
}
#define myFwrite(buf, bufLen) if(outCap < outLen + bufLen){assert(0);}memcpy(out + outLen, buf, bufLen);outLen += bufLen
static int fillTb(char*out, int outCap, char*cell){
	int outLen = 0;
	myFwrite("{", sizeof("{") - 1);
	myFwrite(cell, strlen(cell));
	myFwrite("}", sizeof("}") - 1);
	return outLen;
}
static char isStrDigit(char*p){
	if(!*p){
		return 0;
	}
	for (; *p; ++p){
		if (!(47<*p&&*p<58 || *p == '-')){
			return 0;
		}
	}
	return 1;
}
static int fillRow(char*out, int outCap, char**varType, char**name, char**row, int m, struct shareCell*shareCells, int shareCellsLen, char*sheetname, int*targetIndexs){
	int outLen = 0;
	myFwrite("{", sizeof("{") - 1);
	int j;
	int tj;
	int cnt = 0;
	for(tj = 1; tj < m; ++tj){
		if(targetIndexs) j = targetIndexs[tj];
		else j = tj;
		if(j > 0){
			if(0 < cnt){
				myFwrite(",", sizeof(",") - 1);
			}
			cnt++;
			char* tname = name[tj];
			if(isStrDigit(tname)){
				myFwrite("[", sizeof("[") - 1);
				myFwrite(tname, strlen(tname));
				myFwrite("]", sizeof("]") - 1);
			}else{
				myFwrite(tname, strlen(tname));
			}
			myFwrite("=", sizeof("=") - 1);
			if(varType[j] == "number"){
				if(outCap < outLen + 64){
					assert(0);
				}
				double d = atof(row[j]);
				d = d ? d : strtol(row[j], 0, 0);
				long long ll = 0 < d ? (long long)((d + 1e-8) * 10000) : (long long)((d - 1e-8) * 10000);
				if(ll % 10000 == 0){
					outLen += sprintf(out + outLen, "%.0f", ll / 10000.0);
				}
				else if(ll % 1000 == 0){
					outLen += sprintf(out + outLen, "%.1f", ll / 10000.0);
				}
				else if(ll % 100 == 0){
					outLen += sprintf(out + outLen, "%.2f", ll / 10000.0);
				}
				else if(ll % 10 == 0){
					outLen += sprintf(out + outLen, "%.3f", ll / 10000.0);
				}
				else{
					outLen += sprintf(out + outLen, "%.4f", ll / 10000.0);
				}
			}else if(varType[j] == "plain"){
				myFwrite(row[j], strlen(row[j]));
			}else if(varType[j] == "string"){
				myFwrite("[[", sizeof("[[") - 1);
				myFwrite(row[j], strlen(row[j]));
				myFwrite("]]", sizeof("]]") - 1);
			}else if(varType[j] == "table" || varType[j] == "tbstr"){
				struct shareCell target;
				target.cell = row[j];
				struct shareCell*p = (shareCell *)bsearch(&target, shareCells, shareCellsLen, sizeof(target), cmpCell);
				if(p){
					myFwrite("shareCell[", sizeof("shareCell[") - 1);
					outLen += writeInt(out + outLen, outCap - outLen, p - shareCells + 1);
					myFwrite("]", sizeof("]") - 1);
				}
				else{
					outLen += fillTb(out + outLen, outCap - outLen, row[j]);
				}
			}else{
				puts(sheetname);
				puts(varType[j]);
				puts("number plain string table");
				assert(0);
			}
		}
	}
	myFwrite("}", sizeof("}") - 1);
	return outLen;
}
static char hasChinese(char*p){
	for (; *p; ++p){
		if (!(0<=*p && *p<128)){
			return 1;
		}
	}
	return 0;
}
static int fillSheet(char*out, int outCap, int n, int m, char***vvs, struct shareRow*shareRows, int shareRowsLen, struct shareCell*shareCells, int shareCellsLen, char*sheetname, int*targetIndexs){
	int outLen = 0;
	myFwrite("{", sizeof("{") - 1);
	int i;
	for(i = 2; i < n; ++i){
		if(i < 3){
			myFwrite("\n", sizeof("\n") - 1);
		}
		else{
			myFwrite(",\n", sizeof(",\n") - 1);
		}
		if(isStrDigit(vvs[i][0])){
			myFwrite("[", sizeof("[") - 1);
			myFwrite(vvs[i][0], strlen(vvs[i][0]));
			myFwrite("]", sizeof("]") - 1);
		}else if(hasChinese(vvs[i][0])){
			myFwrite("[\"", sizeof("[\"") - 1);
			myFwrite(vvs[i][0], strlen(vvs[i][0]));
			myFwrite("\"]", sizeof("\"]") - 1);
		}else{
			myFwrite(vvs[i][0], strlen(vvs[i][0]));
		}
		myFwrite("=", sizeof("=") - 1);
		struct shareRow target;
		target.m = m;
		target.name = vvs[1];
		target.row = vvs[i];
		struct shareRow*p = (shareRow *)bsearch(&target, shareRows, shareRowsLen, sizeof(target), cmpRow);
		if(p){
			myFwrite("shareRow[", sizeof("shareRow[") - 1);
			outLen += writeInt(out + outLen, outCap - outLen, p - shareRows + 1);
			myFwrite("]", sizeof("]") - 1);
		}
		else{
			outLen += fillRow(out + outLen, outCap - outLen, vvs[0], vvs[1], vvs[i], m, shareCells, shareCellsLen, sheetname, targetIndexs);
		}
	}
	myFwrite("}", sizeof("}") - 1);	
	return outLen;
}
static bool isFixPatternName(char* name, char** patternNames, int patternNameLen){
	if(!name) return false;
	int i;
	for(i = 0; i < patternNameLen; i ++) {
		if(strcmp(name, patternNames[i]) == 0) return true;
	}
	return false;
} 
static bool myStringCmp(char* str1, int st1, int ed1, char* str2, int st2, int ed2){
	if(!str1||st1 < 0 || ed1 >= strlen(str1)) return false;
	if(!str1||st2 < 0 || ed2 >= strlen(str2)) return false;
	if((ed1-st1)!=(ed2-st2)) return false;
	int i = st1;
	int j = st2;
	for(;i <= ed1; i++, j++) {
		if(str1[i]!=str2[j]) return false;
	}
	return true;
}
static int isFixPatternNameR(char* rname, char** patternNames, int patternNameLen) {
	int i;
	for(i = patternNameLen - 1; i > 0; i--) {
		char* patternName = patternNames[i]; // 匹配后缀.
		int plen = strlen(patternName);
		if(myStringCmp(rname, strlen(rname)-plen, strlen(rname)-1, patternName, 0, plen-1)) return i;
	}
	return 0;
}
static bool isFixPatternNameL(char* name1, char* name2, char* pname) {
	int len1 = strlen(name1);
	int len2 = strlen(name2);
	int len3 = strlen(pname);

	return myStringCmp(name1, 0, len1-1, name2, 0, len2-len3-1);
}
static void solvePattern(char* targetName, char** patternNames, int patternNameLen, char** rnames, int m, int* targetIndexs){
	int i, j;
	int ptype;
	char* rname;
	for(i = 1; i < m; i++) {
		rname = rnames[i]; // 表头名字.
		ptype = isFixPatternNameR(rname, patternNames, patternNameLen);
		if(ptype > 0){
			targetIndexs[i] = -ptype;
		}
	}
	if(targetName && strcmp(targetName, patternNames[0]) != 0) { //使用其它项目的内容.
		ptype = isFixPatternNameR(targetName, patternNames, patternNameLen);
		if(strcmp(targetName, patternNames[ptype]) == 0) {
			for(i = 1; i < m; i++) {
				if(targetIndexs[i] == -ptype) {
					for(j = 1; j < m; j++) {
						if(targetIndexs[j] > 0 && isFixPatternNameL(rnames[j], rnames[i], patternNames[ptype])){
							targetIndexs[j] = i;
						}
					}
				}
			}
		}
	}
}
#undef myFwrite
// argv说明：1-输入xlsx文件 2-输出lua文件 3-noshare 4-替换后缀.
int main(int argc, char**argv){
	FILE*fileIn = fopen(argv[1], "rb");
	if(!fileIn){
		printf("文件名错误，找不到文件[%s]\n", argv[1]);
		assert(0);
	}
	char noshare = argv[3] && !strcmp(argv[3],"noshare");
	struct tri xmlFiles[512];
	int xmlFilesLen = 0;
	char*rules[] = {"xl/sharedStrings.xml", "xl/worksheets/sheet", "xl/workbook.xml"};
	for(;;){
#pragma pack(1)
		struct{
			int localFileHeaderSignature;
			short versionNeededToExtract;
			short generalPurposeBitFlag;
			short compressionMethod;
			short lastModFileTime;
			short lastModFileDate;
			int crc;
			int compressedSize;
			int uncompressedSize;
			short fileNameLength;
			short extraFieldLength;
		}h;
#pragma pack()
		h.localFileHeaderSignature = 0;
		fread(&h, sizeof(h), 1, fileIn);
		if(h.localFileHeaderSignature - 0x04034b50){
			break;
		}
		assert(h.versionNeededToExtract == 0x14 || h.versionNeededToExtract == 0xa);
		assert(h.generalPurposeBitFlag == 0x6 || h.generalPurposeBitFlag == 0x0);
		assert(h.compressionMethod == 0x8 || h.compressionMethod == 0x0);
		// assert(h.lastModFileTime == 0x0);
		// assert(h.lastModFileDate == 0x21);
		assert(h.compressionMethod || h.compressedSize == h.uncompressedSize);
		if(sizeof(xmlFiles) / sizeof(*xmlFiles) <= xmlFilesLen){
			assert(0);
		}
		struct tri*t = xmlFiles + xmlFilesLen;
		if(sizeof(t->filename) <= h.fileNameLength){
			assert(0);
		}
		if(fread(t->filename, 1, h.fileNameLength, fileIn) - h.fileNameLength){
			assert(0);
		}
		t->filename[h.fileNameLength] = 0;
		if(fseek(fileIn, h.extraFieldLength, SEEK_CUR)){
			assert(0);
		}
		if(4<=h.fileNameLength && !memcmp(t->filename + h.fileNameLength - 4, ".xml", 4)){
			int i;
			for(i = 0; i < sizeof(rules) / sizeof(*rules); ++i){
				int rulesILen = strlen(rules[i]);
				if(h.fileNameLength < rulesILen || memcmp(t->filename, rules[i], rulesILen)){
					continue;
				}
				++xmlFilesLen;
				t->compressionMethod = h.compressionMethod;
				t->compressedSize = h.compressedSize;
				t->uncompressedSize = h.uncompressedSize;
				t->offset = ftell(fileIn);
				break;
			}
		}
		if(fseek(fileIn, h.compressedSize, SEEK_CUR)){
			assert(0);
		}
	}
	static char out[1 << 26];
	fileStr(out, sizeof(out), "xl/workbook.xml", fileIn, xmlFiles, xmlFilesLen);
	char*workbook[512];
	getWorkbook(workbook, sizeof(workbook)/sizeof(*workbook), out);
	fileStr(out, sizeof(out), "xl/sharedStrings.xml", fileIn, xmlFiles, xmlFilesLen);
	char*sharedStrings[65536];
	getSharedStrings(sharedStrings, sizeof(sharedStrings) / sizeof(*sharedStrings), out);
	int i;
	char***svvs[sizeof(workbook)/sizeof(*workbook)];
	int nn[sizeof(workbook)/sizeof(*workbook)];
	int mm[sizeof(workbook)/sizeof(*workbook)];	
	struct shareSheet shareSheets[sizeof(workbook) / sizeof(*workbook)];
	int targetIndexs[sizeof(workbook)/sizeof(*workbook)];
	int shareSheetsLen = 0;
	for (i = 0; workbook[i]; ++i){
		if (hasChinese(workbook[i])){
			continue;
		}
		char buf[64] = "xl/worksheets/sheet";
		int bufLen = sizeof("xl/worksheets/sheet") - 1;
		bufLen += writeInt(buf + bufLen, sizeof(buf) - bufLen, i + 1);
		memcpy(buf + bufLen, ".xml", sizeof(".xml"));
		fileStr(out, sizeof(out), buf, fileIn, xmlFiles, xmlFilesLen);
		int n, m;
		char***vvs = getVvs(&n, &m, sharedStrings, out, workbook[i], argv[1]);
		svvs[i] = vvs;
		nn[i] = n;
		mm[i] = m;

		struct shareSheet target;
		target.n = n;
		target.m = m;
		target.cnt = 1;
		target.sheet = vvs;
#define binInsert(share, shareLen, cmp)\
		int a = 0, b = shareLen - 1;\
		while(a <= b){\
			int c = a + b >> 1;\
			int ret = cmp(&target, share + c);\
			if(ret < 0){\
				b = c - 1;\
			}\
			else if(0 < ret){\
				a = c + 1;\
			}\
			else{\
				++share[c].cnt;\
				break;\
			}\
		}\
		if(b < a){\
			if(sizeof(share) / sizeof(*share) <= shareLen){\
				assert(0);\
			}\
			memmove(share + a + 1, share + a, sizeof(*share) * (shareLen - a));\
			++shareLen;\
			share[a] = target;\
		}
		binInsert(shareSheets, shareSheetsLen, cmpSheet);
	}
	static struct shareRow shareRows[32768];
	int shareRowsLen = 0;
	for(i = 0; i < shareSheetsLen; ++i){
		struct shareSheet*t = shareSheets + i;
		int n = t->n;
		int m = t->m;
		char***sheet = t->sheet;
		char**sheet0 = sheet[0];
		char**sheet1 = sheet[1];
		int i;
		for(i = 2; i < n; ++i){
			struct shareRow target;
			target.m = m;
			target.cnt = 1;
			target.varType = sheet0;
			target.name = sheet1;
			target.row = sheet[i];
			binInsert(shareRows, shareRowsLen, cmpRow);
		}
	}
	struct shareCell shareCells[32768];
	int shareCellsLen = 0;
	for(i = 0; i < shareRowsLen; ++i){
		struct shareRow*t = shareRows + i;
		char**varType = t->varType;
		int m = t->m;
		char**row = t->row;
		int i;
		for(i = 1; i < m; ++i){
			if(varType[i] == "table" || varType[i] == "tbstr"){
				struct shareCell target;
				target.cell = row[i];
				target.cnt = 1;
				binInsert(shareCells, shareCellsLen, cmpCell);
			}
		}
	}
	int top = noshare?0x7fffffff:1;
	int j = 0;
	for(i = 0; i < shareCellsLen; ++i){
		if(top < shareCells[i].cnt){
			shareCells[j++] = shareCells[i];
		}
	}
	shareCellsLen = j;
	j = 0;
	for(i = 0; i < shareRowsLen; ++i){
		if(top < shareRows[i].cnt){
			shareRows[j++] = shareRows[i];
		}
	}
	shareRowsLen = j;
	j = 0;
	for(i = 0; i < shareSheetsLen; ++i){
		if(top < shareSheets[i].cnt){
			shareSheets[j++] = shareSheets[i];
		}
	}
	shareSheetsLen = j;
	// 需要处理的后缀名列表.
	char* patternNames[] = {"", "_bt2", "_tw", "_en", "_vi", "_kr", "_jp"};
	int patternNameLen = sizeof(patternNames) / sizeof(*patternNames);
	int outLen = 0;
#define myFwrite(buf, bufLen) if(sizeof(out) < outLen + bufLen){assert(0);}memcpy(out + outLen, buf, bufLen);outLen += bufLen
	if(!argv[4]||strcmp(argv[4],"check")!=0){
		for(i = 0; i < shareCellsLen; ++i){
			if(i < 1){
				myFwrite("local shareCell={\n[", sizeof("local shareCell={\n[") - 1);
			}
			else{
				myFwrite(",\n[", sizeof(",\n[") - 1);
			}
			outLen += writeInt(out + outLen, sizeof(out) - outLen, i + 1);
			myFwrite("]=", sizeof("]=") - 1);
			struct shareCell*share = shareCells + i;
			outLen += fillTb(out + outLen, sizeof(out) - outLen, share->cell);
			if(i + 1 == shareCellsLen){
				myFwrite("}\n", sizeof("}\n") - 1);
			}
		}
		for(i = 0; i < shareRowsLen; ++i){
			if(i < 1){
				myFwrite("local shareRow={\n[", sizeof("local shareRow={\n[") - 1);
			}
			else{
				myFwrite(",\n[", sizeof(",\n[") - 1);
			}
			outLen += writeInt(out + outLen, sizeof(out) - outLen, i + 1);
			myFwrite("]=", sizeof("]=") - 1);
			struct shareRow*share = shareRows + i;
			outLen += fillRow(out + outLen, sizeof(out) - outLen, share->varType, share->name, share->row, share->m, shareCells, shareCellsLen, "share", NULL);
			if(i + 1 == shareRowsLen){
				myFwrite("}\n", sizeof("}\n") - 1);
			}
		}
		for(i = 0; i < shareSheetsLen; ++i){
			if(i < 1){
				myFwrite("local shareSheet={\n[", sizeof("local shareSheet={\n[") - 1);
			}
			else{
				myFwrite(",\n[", sizeof(",\n[") - 1);
			}
			outLen += writeInt(out + outLen, sizeof(out) - outLen, i + 1);
			myFwrite("]=", sizeof("]=") - 1);
			struct shareSheet*share = shareSheets + i;
			outLen += fillSheet(out + outLen, sizeof(out) - outLen, share->n, share->m, share->sheet, shareRows, shareRowsLen, shareCells, shareCellsLen, "share", NULL);
			if(i + 1 == shareSheetsLen){
				myFwrite("}\n", sizeof("}\n") - 1);
			}
		}
		char rn = 0;
		for (i = 0; workbook[i]; ++i){
			if (hasChinese(workbook[i])){
				continue;
			}
			char ***vvs = svvs[i];
			int n = nn[i];
			int m = mm[i];
			if(rn){
				myFwrite("\n", sizeof("\n") - 1);
			}
			else{
				rn = 1;
			}
			if(noshare){
				myFwrite("(function()", sizeof("(function()") - 1);
			}
			if(isStrDigit(workbook[i])){
				myFwrite("getfenv()[", sizeof("getfenv()[") - 1);
				myFwrite(workbook[i], strlen(workbook[i]));
				myFwrite("]", sizeof("]") - 1);
			}else{
				myFwrite(workbook[i], strlen(workbook[i]));
			}
			myFwrite("=", sizeof("=") - 1);
			struct shareSheet target;
			target.n = n;
			target.m = m;
			target.sheet = vvs;
			struct shareSheet*p = (shareSheet *)bsearch(&target, shareSheets, shareSheetsLen, sizeof(target), cmpSheet);
			if(p){
				myFwrite("shareSheet[", sizeof("shareSheet[") - 1);
				outLen += writeInt(out + outLen, sizeof(out) - outLen, p - shareSheets + 1);
				myFwrite("]", sizeof("]") - 1);
			}
			else{
				// 字段替换 start			
				for(j = 0; j < m; j++) targetIndexs[j] = j;
				if(isFixPatternName(argv[4], patternNames, patternNameLen)){
					solvePattern(argv[4], patternNames, patternNameLen, vvs[1], m, targetIndexs);				
				}
				if(isFixPatternName(argv[5], patternNames, patternNameLen)){
					solvePattern(argv[5], patternNames, patternNameLen, vvs[1], m, targetIndexs);				
				}
				// 字段替换 end

				outLen += fillSheet(out + outLen, sizeof(out) - outLen, n, m, vvs, shareRows, shareRowsLen, shareCells, shareCellsLen, workbook[i], targetIndexs);
			}
			if(noshare){
				myFwrite("end)();", sizeof("end)();") - 1);
			}
		}
		FILE*fileOut = fopen(argv[2], "wb");
		if(!fileOut){
			printf("路径错误,找不到[%s]\n", argv[2]);
			assert(0);
		}
		fwrite(out, 1, outLen, fileOut);
	} else{
		for (i = 0; workbook[i]; ++i){
			if (hasChinese(workbook[i])){
				continue;
			}
			char ***vvs = svvs[i];
			int n = nn[i];
			int m = mm[i];
			for(j = 0; j < m; j++) {
				int ptype = isFixPatternNameR(vvs[1][j], patternNames, patternNameLen);
				if(ptype > 0){
					myFwrite(argv[1], strlen(argv[1]));
					myFwrite("/", strlen("/"));
					myFwrite(workbook[i], strlen(workbook[i]));
					myFwrite("/", strlen("/"));
					myFwrite(vvs[1][j], strlen(vvs[1][j]));
					myFwrite("\n", strlen("\n"));
				}
			}
		}

		FILE*fileOut = fopen("./check.txt", "a");
		if(!fileOut){
			printf("路径错误,找不到[%s]\n", argv[2]);
			assert(0);
		}
		fwrite(out, 1, outLen, fileOut);
	}	

	return 0;
}
