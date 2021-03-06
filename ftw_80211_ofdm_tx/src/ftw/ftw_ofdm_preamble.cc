/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ftw_ofdm_preamble.h>
#include <gr_io_signature.h>
#include <stdexcept>
#include <iostream>
#include <string.h>

ftw_ofdm_preamble_sptr
ftw_make_ofdm_preamble(int symbol_length, int N_symbols,
		const std::vector<std::vector<gr_complex> > &preamble)
{
  return ftw_ofdm_preamble_sptr(new ftw_ofdm_preamble(symbol_length, N_symbols, preamble));
}

ftw_ofdm_preamble::ftw_ofdm_preamble(int symbol_length, int N_symbols,
			const std::vector<std::vector<gr_complex> > &preamble) : gr_block("preamble",
	     		gr_make_io_signature2(2, 2, sizeof(gr_complex)*(symbol_length), sizeof(char)),
	     		gr_make_io_signature2(1, 2, sizeof(gr_complex)*symbol_length, sizeof(char))),
  d_symbol_length(symbol_length),
  d_N_symbols(N_symbols), 
  d_preamble(preamble),
  d_state(ST_IDLE),
  d_nsymbols_output(0),
  d_pending_flag(0)
{
  set_output_multiple(d_N_symbols + 1);   // (320 samples training sequences + 80 samples first signal ofdm symbol = 400)--> 400 / 80 = 5
  enter_idle();
}


ftw_ofdm_preamble::~ftw_ofdm_preamble(){}

int ftw_ofdm_preamble::general_work (int noutput_items,
				gr_vector_int &ninput_items_v,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items)
{
  const gr_complex *in_sym = (const gr_complex *) input_items[0];
  const unsigned char *in_flag = (const unsigned char *) input_items[1];
  gr_complex *out_sym = (gr_complex *) output_items[0];

  int no = 0;	// number items output
  int ni = 0;	// number items read from input

  switch(d_state){
    case ST_IDLE:
      if (in_flag[ni])	// this is the first symbol of the new payload
	enter_first_payload_and_preamble();
      else
	ni++;			// eat one input symbol
      break;

    	
    case ST_FIRST_PAYLOAD_AND_PREAMBLE:
      for(int temp=0; temp<4; temp++)
      {
         memcpy(&out_sym[no * d_symbol_length], &d_preamble[d_nsymbols_output][no * d_symbol_length],d_symbol_length * sizeof(gr_complex));
         no++;
      }
      while(no < noutput_items+4){
      // copy first payload symbol from input to output
        memcpy(&out_sym[no * d_symbol_length],&in_sym[ni * d_symbol_length],d_symbol_length * sizeof(gr_complex));
        no++;
	ni++;
      }
      break; 	


    default:
      std::cerr << "gr_preamble: (can't happen) invalid state, resetting\n";
      enter_idle();
  }
  consume_each(ni);
  return no;
}

void ftw_ofdm_preamble::enter_idle(){
  d_state = ST_IDLE;
  d_nsymbols_output = 0;
  d_pending_flag = 0;
}

void ftw_ofdm_preamble::enter_first_payload_and_preamble(){
  d_state = ST_FIRST_PAYLOAD_AND_PREAMBLE;
}

