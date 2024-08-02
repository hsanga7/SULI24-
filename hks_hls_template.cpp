#include "hks_hls_template.h"
const int number_of_groups = 5;

ap_uint<1> multi_trg(
		ap_uint<256> fadc_hits,
		ap_uint<9> multiplicty_thr
)
{
	ap_uint<9> hit_cnt = 0;

	for(int i=0;i<256;i++)
		hit_cnt+= fadc_hits[i] ? 1 : 0;

	if(hit_cnt >= multiplicty_thr)
		return 1;

	return 0;
}

ap_uint<1> or_trg(
		ap_uint<256> fadc_hits
)
{
	return fadc_hits.or_reduce();
}

template <int N>
ap_uint<N> pulse_stretch(
		ap_uint<N> fadc_hits,
		ap_uint<8> stretch_width,
		ap_uint<8> counter[N]
)
{
	ap_uint<N> fadc_hits_s;
	for (int n = 0; n < N; n++)
	{
		if (fadc_hits[n])
		{
			fadc_hits_s[n] = 1;
			counter[n] = stretch_width;
		}
		else if (counter[n] > 0)
		{
			counter[n]--;
			fadc_hits_s[n] = 1;
		}
		else
		{
			fadc_hits_s[n] = 0;
		}
	}
	return fadc_hits_s;
}

template<int A>
ap_uint<A/2> segment(
		ap_uint<A>fadc_hits_seg
)
{
	ap_uint<A/2>fadc_hits;

	for (int n = 0; n < ((A/2)); n++){
		int a= 2*n;
		int b =a+1;
		fadc_hits[n]=(fadc_hits_seg[a] && fadc_hits_seg[(b)]);
	}
	return fadc_hits;
}

template <int B>
void grouping(
		ap_uint<1>num[],
		ap_uint<B> fadc_hits,
		int identifier)
{
	int a1,a2,b1,b2,c1,c2,d1,d2,e1,e2;

	switch(identifier){

	case 0:				//ktof1x
		a1=  0; a2= 3;
		b1=  4; b2= 6;
		c1=  7; c2= 9;
		d1= 10; d2= 12;
		e1= 13; e2= 16;
		break;
	case 2:             //ac1
	case 3:				//ac2
	case 4:				//ac3
		a1= 0; a2= 5;
		b1= 0; b2= 5;
		c1= 0; c2= 6;
		d1= 1; d2= 6;
		e1= 1; e2= 6;
		break;
	case 5:				//ktof2x
		a1=  0; a2=  5;
		b1=  3; b2=  8;
		c1=  6; c2= 11;
		d1=  9; d2= 14;
		e1= 12; e2= 17;
		break;
	case 6:				//wc1
		a1= 0; a2=  4;
		b1= 1; b2=  8;
		c1= 3; c2= 10;
		d1= 4; d2= 11;
		e1= 5; e2= 11;
		break;
	case 7:				//wc2
		a1= 0; a2= 6;
		b1= 1; b2= 9;
		c1= 3; c2= 11;
		d1= 5; d2= 11;
		e1= 6; e2= 11;
		break;
	default:

		return;

	}

	num[0]=fadc_hits.range(a2, a1).or_reduce();
	num[1]=fadc_hits.range(b2, b1).or_reduce();
	num[2]=fadc_hits.range(c2, c1).or_reduce();
	num[3]=fadc_hits.range(d2, d1).or_reduce();
	num[4]=fadc_hits.range(e2, e1).or_reduce();



}

void hks_hls_template(
		ap_uint<8> stretch_width[8],
		ap_uint<9> multiplicty_thr,
		hls::stream<fadc_vxs_hits_t> &s_fadc_vxs_hits,
		hls::stream<trig_t> &s_trig
)
{
#pragma HLS INTERFACE ap_fifo port=s_trig
#pragma HLS INTERFACE ap_fifo port=s_fadc_vxs_hits
#pragma HLS stable variable = multiplicty_thr
#pragma HLS stable variable = stretch_width
#pragma HLS PIPELINE II=1 style=flp

	trig_t trig;
	fadc_vxs_hits_t fadc_vxs_hits = s_fadc_vxs_hits.read();

	// since we're running with 31.25MHz clock, but
	// have 4ns resolution data we must perform 8 iterations per clock cycle

	for(int t=0;t<8;t++)
	{
		ap_uint<256> fadc_hits;
		for(int i=0;i<256;i++)
			fadc_hits[i] = fadc_vxs_hits.hits[i].hit[t];

		// you will likely want some kind of pulse stretching logic performed
		// before calling the trigger bit algorithms

		//Assigning delay value for each detector separately
		//They are sequenced in the same order as the experimental setup
		//{0-8}={ktof1x,ktof2x,ac1,ac2,ac3,ktof1y, wc1,wc2}

		//Assigning detector hit signals each segment has 2 consecutive channels
		// detector 1 segment 1 has ch0 & ch1, detector 1 segment 2 uses ch3 & ch4


		ap_uint<34>ktof1x_seg=fadc_hits.range(33,0);      	//34 channel......17 segments each with 2 channels
		ap_uint<17>ktof1x = segment(ktof1x_seg);  			//De-segment stream
		ap_uint<8> counter_ktof1x[17] = {};
		ap_uint<17> ktof1x_s = pulse_stretch(ktof1x, stretch_width[0], counter_ktof1x); // Stretched and de-segmented value


		ap_uint<18> ktof1y_seg=fadc_hits.range(51,34);      //18Channels....9 Segments
		ap_uint<9> ktof1y_h=segment(ktof1y_seg);			//
		ap_uint<8> counter_ktof1y[9] = {};					//
		ap_uint<9> ktof1y_s = pulse_stretch(ktof1y_h, stretch_width[1], counter_ktof1y);
		ap_uint<1> ktof1y=ktof1y_s.or_reduce(); 			//TOF1y is not grouped


		ap_uint<14>ac1_seg=fadc_hits.range(65,52);        //14channels
		ap_uint<8> counter_ac1[7] = {};
		ap_uint<7>ac1=segment(ac1_seg);
		ap_uint<7> ac1_s = pulse_stretch(ac1, stretch_width[2], counter_ac1);

		ap_uint<14>ac2_seg=fadc_hits.range(79,66);		//14 Channels
		ap_uint<8> counter_ac2[7] = {};
		ap_uint<7>ac2=segment(ac2_seg);
		ap_uint<7> ac2_s = pulse_stretch(ac2, stretch_width[3], counter_ac2);

		ap_uint<14>ac3_seg=fadc_hits.range(93,80);		//14 Channels
		ap_uint<7>ac3=segment(ac3_seg);
		ap_uint<8> counter_ac3[7] = {};
		ap_uint<7> ac3_s = pulse_stretch(ac3, stretch_width[4], counter_ac3);

		ap_uint<36>ktof2x_seg=fadc_hits.range(129,94);    //36 Channels
		ap_uint<18>ktof2x=segment(ktof2x_seg);
		ap_uint<8> counter_ktof2x[18] = {};
		ap_uint<18> ktof2x_s = pulse_stretch(ktof2x, stretch_width[5], counter_ktof2x);

		ap_uint<24> wc1_seg=fadc_hits.range(153,130);		// 24 Channels
		ap_uint<7>  wc1=segment(wc1_seg);
		ap_uint<8>  counter_wc1[12] = {};
		ap_uint<12> wc1_s = pulse_stretch(wc1, stretch_width[6], counter_wc1);

		ap_uint<24>wc2_seg=fadc_hits.range(177,154);		// 24 Channels
		ap_uint<12>wc2=segment(wc2_seg);
		ap_uint<8> counter_wc2[12] = {};
		ap_uint<12> wc2_s = pulse_stretch(wc2, stretch_width[7], counter_wc2);


		//grouping and checking hit status of each individual group

		ap_uint<1> num_ktof1x[number_of_groups];
		grouping(num_ktof1x,ktof1x,0);

		ap_uint<1> num_ac1[number_of_groups];
		grouping(num_ac1, ac1,2);

		ap_uint<1> num_ac2[number_of_groups];
		grouping(num_ac2, ac2,3);

		ap_uint<1> num_ac3[number_of_groups];
		grouping(num_ac3, ac3,4);

		ap_uint<1> num_ktof2x[number_of_groups];
		grouping(num_ktof2x, ktof2x,5);

		ap_uint<1> num_wc1[number_of_groups];
		grouping(num_wc1, wc1,6);

		ap_uint<1> num_wc2[number_of_groups];
		grouping(num_wc2, wc2,7);


		// trigger logic implementation

		//Individual groups' trigger

		ap_uint<number_of_groups> grp_trg=0;

		for(int k=0;k<number_of_groups; k++)
		{

			grp_trg[k]=(((num_ktof1x[k])&&(num_ktof2x[k])&&(ktof1y)) &&
					((num_ac1[k] && num_ac2[k]))||(num_ac1[k] && num_ac3[k]) || (num_ac2[k] && num_ac3[k]) &&
					(num_wc1[k] && num_wc2[k])) ;;

		}

		// Evaluating hits across all groups

		ap_uint<1>hks_trg=grp_trg.or_reduce();

		// Trigger bit 0: multiplicty trigger
		trig.trg[0][t] = multi_trg(fadc_hits, multiplicty_thr);

		// Trigger bit 1: or trigger
		trig.trg[1][t] = or_trg(fadc_hits);

		// Trigger bit 2: group trigger status
		trig.trg[2][t] = grp_trg;

		// Trigger bit 3: hks trigger
		trig.trg[3][t] = hks_trg;

		//Trigger bit 4 to 5: unused

	}

	// write 32ns worth of trigger decisions
	s_trig.write(trig);
}
