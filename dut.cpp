#include <cmath>
#include "stratus_hls.h"

#include "dut.h"

dut::dut(sc_module_name n) : sc_module(n)
{

	// HLS_FLATTEN_ARRAY(val);
	SC_THREAD(do_filter);
	sensitive << i_clk.pos();
	dont_initialize();
	reset_signal_is(i_rst, false);

	i_rgb.clk_rst(i_clk, i_rst);
	o_result.clk_rst(i_clk, i_rst);
}

dut::~dut() {}

// sobel mask
const int mask[MASK_X][MASK_Y] = {{1, 1, 1}, {1, 2, 1}, {1, 1, 1}};

void dut::do_filter()
{
	{

		HLS_DEFINE_PROTOCOL("main_reset");
		i_rgb.reset();
		o_result.reset();

		wait();
	}

	while (true)
	{
#if defined(II)
		HLS_PIPELINE_LOOP(HARD_STALL, II, "Loop");
#endif
		sc_uint<32> a[9];

		for (unsigned int i = 0; i < 9; i++)
		{
			sc_dt::sc_uint<24> rgb;
			{
				HLS_DEFINE_PROTOCOL("input");
				rgb = i_rgb.get();
				wait();
			}

			unsigned char grey = (rgb.range(7, 0) + rgb.range(15, 8) + rgb.range(23, 16)) / 3;
			buffer[i] = grey;
		}

		for (int i = 0; i < 9; i++)
		{
			a[i] = buffer[i];
		}

		{ // sorting
			bool swapped = true;
			sc_uint<4> j = 0;

			while (swapped)
			{
				swapped = false;
				j++;
				for (int i = 0; i < 9 - j; ++i)
				{
					if (buffer[i] > buffer[i + 1])
					{
						sc_uint<32> temp = buffer[i];
						buffer[i] = buffer[i + 1];
						buffer[i + 1] = temp;
						swapped = true;
					}
				}
			}
		}
		a[4] = buffer[4];

		{
			// 刪除buffer裡的資料
			for (int i = 0; i < 9; i++)
				buffer[i] = 0;
		}

		sc_uint<4> total = 0;
		sc_uint<4> k = 0; // Mean filter
		for (unsigned int v = 0; v < MASK_Y; ++v)
		{
			for (unsigned int u = 0; u < MASK_X; ++u)
			{
				// HLS_CONSTRAIN_LATENCY(0, 1, "lat00");
				total += a[k] * mask[v][u];
				k++;
			}
		}

		{
			HLS_DEFINE_PROTOCOL("output");
			sc_uint<32> result = (total / 10);
			o_result.put(result);
			wait();
		}
	}
}
