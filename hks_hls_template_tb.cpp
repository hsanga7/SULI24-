#include <stdlib.h>
#include <stdio.h>
#include "hks_hls_template.h"

#define TB_NUM_32NS_SAMPLES   5
#define HIT_P                 0.0001
#define MULTIPLICITY_THR      5



int main()
{
	ap_uint<9> multiplicty_thr = MULTIPLICITY_THR;
	ap_uint<8> stretch_width[8]= {0,0,0,0,0,0,0,0};
	hls::stream<fadc_vxs_hits_t> s_fadc_vxs_hits;
	hls::stream<trig_t> s_trig;
	hls::stream<trig_t> s_trig_answer;
	ap_uint<256> fadc_hits = 0;
	int result = 0;

	for(int i=0; i<TB_NUM_32NS_SAMPLES; i++)
	{
		fadc_vxs_hits_t fadc_vxs_hits;
		trig_t trig_answer;

		for(int t=0; t<8; t++)
		{
			ap_uint<1> trig_answer_or = 0;
			ap_uint<1> trig_answer_mult = 0;

			for(int trg=0; trg<TRIGGER_NUM; trg++)
				trig_answer.trg[trg][t] = 0;

			int mult_cnt = 0;
			for(int ch=0; ch<256; ch++)
			{
				fadc_vxs_hits.hits[ch].hit[t] = ( (std::rand() % 48) /48.0) ? 1 : 0;
				if(fadc_vxs_hits.hits[ch].hit[t])
				{
					mult_cnt++;
					fadc_hits[ch]=1;
				}
				else{
					fadc_hits[ch]=0;
				}
			}



			//segements
			ap_uint<128>fadc_hit;

			for (int n = 0; n < 128; n++)
			{
				int a= 2*n;
				int b =a+1;
				fadc_hit[n]=(fadc_hits[a] && fadc_hits[b]);
			}
			//grouping
			ap_uint<5>grp;
			//group 1
			grp[0]=(fadc_hit.range(3,0).or_reduce()&&
					fadc_hit.range(22,17).or_reduce() &&
					fadc_hit.range(40,35).or_reduce() &&
					fadc_hit.range(47,42).or_reduce() &&
					fadc_hit.range(54,49).or_reduce() &&
					fadc_hit.range(64,56).or_reduce() &&
					fadc_hit.range(69,65).or_reduce() &&
					fadc_hit.range(82,77).or_reduce());

			grp[1]=(fadc_hit.range(6,4).or_reduce()&&
					fadc_hit.range(25,20).or_reduce() &&
					fadc_hit.range(40,35).or_reduce() &&
					fadc_hit.range(47,42).or_reduce() &&
					fadc_hit.range(54,49).or_reduce() &&
					fadc_hit.range(64,56).or_reduce() &&
					fadc_hit.range(73,66).or_reduce() &&
					fadc_hit.range(86,78).or_reduce());

			grp[2]=(fadc_hit.range(9,7).or_reduce()&&
					fadc_hit.range(28,23).or_reduce() &&
					fadc_hit.range(41,35).or_reduce() &&
					fadc_hit.range(48,42).or_reduce() &&
					fadc_hit.range(55,49).or_reduce() &&
					fadc_hit.range(64,56).or_reduce() &&
					fadc_hit.range(75,68).or_reduce() &&
					fadc_hit.range(88,80).or_reduce());

			grp[3]=(fadc_hit.range(12,10).or_reduce()&&
					fadc_hit.range(31,26).or_reduce() &&
					fadc_hit.range(41,36).or_reduce() &&
					fadc_hit.range(48,43).or_reduce() &&
					fadc_hit.range(55,50).or_reduce() &&
					fadc_hit.range(64,56).or_reduce() &&
					fadc_hit.range(76,69).or_reduce() &&
					fadc_hit.range(88,82).or_reduce());

			grp[4]=(fadc_hit.range(16,13).or_reduce()&&
					fadc_hit.range(34,29).or_reduce() &&
					fadc_hit.range(41,36).or_reduce() &&
					fadc_hit.range(48,43).or_reduce() &&
					fadc_hit.range(55,50).or_reduce() &&
					fadc_hit.range(64,56).or_reduce() &&
					fadc_hit.range(76,70).or_reduce() &&
					fadc_hit.range(88,83).or_reduce());




			trig_answer.trg[0][t] = (mult_cnt >= multiplicty_thr) ? 1 : 0;
			trig_answer.trg[1][t] = (mult_cnt > 0)                ? 1 : 0;
			trig_answer.trg[2][t] = grp;
			trig_answer.trg[3][t] = grp.or_reduce();

		}
		s_trig_answer.write(trig_answer);
		s_fadc_vxs_hits.write(fadc_vxs_hits);
	}

	while(!s_fadc_vxs_hits.empty())
		hks_hls_template(stretch_width,multiplicty_thr, s_fadc_vxs_hits, s_trig);

	printf("s_trig_answer.size() = %d\n", s_trig_answer.size());
	printf("s_trig.size() = %d\n", s_trig.size());

	if(s_trig_answer.size() != s_trig.size())
	{
		printf("Error: s_trig_answer.size() != s_trig.size()\n");
		result = -1;
	}
	else
	{
		int t_32ns = 0;
		while(!s_trig.empty())
		{
			trig_t trig = s_trig.read();
			trig_t trig_answer = s_trig_answer.read();

			for(int t=0; t<8; t++)
			{
				for(int trg=0; trg<TRIGGER_NUM; trg++)
				{
					if(trig.trg[trg][t] != trig_answer.trg[trg][t])
					{
						printf("Error: @T=%d, trig[%d]=%d, trig_answer=%d\n", t_32ns+t, trg, (int)trig.trg[trg][t], (int)trig_answer.trg[trg][t]);
						result = -1;
					}
				}
			}
			t_32ns++;
		}
	}

	return result;
}
