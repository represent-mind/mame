// license:BSD-3-Clause
// copyright-holders:Curt Coder
/***************************************************************************

C-80

Pasting:
    0-F : as is
    + (inc) : ^
    - (dec) : V
    M : -
    GO : X

Test Paste:
    -800^11^22^33^44^55^66^77^88^99^-800
    Now press up-arrow to confirm the data has been entered.

Commands:
    R : REGister
    M : MEMory manipulation
    G : GO
  F10 : RESet
  ESC : BRK

Functions (press F1 then the indicated number):
    0 : FILL
    1 : SAVE
    2 : LOAD
    3 : LOADP
    4 : MOVE
    5 : IN
    6 : OUT

When REG is chosen, use UP to scroll through the list of regs,
or press 0 thru C to choose one directly:
    0 : SP
    1 : PC
    2 : AF
    3 : BC
    4 : DE
    5 : HL
    6 : AF'
    7 : BC'
    8 : DE'
    9 : HL'
    A : IFF
    B : IX
    C : IY

When MEM is chosen, enter the address, press UP, enter data, press UP, enter
data of next byte, and so on.

****************************************************************************/

#include "emu.h"
#include "includes/c80.h"

#include "sound/wave.h"
#include "speaker.h"

#include "c80.lh"


/* Memory Maps */

void c80_state::c80_mem(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x07ff).rom();
	map(0x0800, 0x0bff).mirror(0x400).ram();
}

void c80_state::c80_io(address_map &map)
{
	map.global_mask(0xff);
	map(0x7c, 0x7f).rw(Z80PIO2_TAG, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
	map(0xbc, 0xbf).rw(m_pio1, FUNC(z80pio_device::read), FUNC(z80pio_device::write));
}

/* Input Ports */

INPUT_CHANGED_MEMBER( c80_state::trigger_reset )
{
	m_maincpu->set_input_line(INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

INPUT_CHANGED_MEMBER( c80_state::trigger_nmi )
{
	m_maincpu->set_input_line(INPUT_LINE_NMI, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( c80 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REG") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GO") PORT_CODE(KEYCODE_G) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FCN") PORT_CODE(KEYCODE_F1)

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("+") PORT_CODE(KEYCODE_UP) PORT_CHAR('^')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("-") PORT_CODE(KEYCODE_DOWN) PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MEM") PORT_CODE(KEYCODE_M) PORT_CHAR('-')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RES") PORT_CODE(KEYCODE_F10) PORT_CHANGED_MEMBER(DEVICE_SELF, c80_state, trigger_reset, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BRK") PORT_CODE(KEYCODE_ESC) PORT_CHANGED_MEMBER(DEVICE_SELF, c80_state, trigger_nmi, 0)
INPUT_PORTS_END

/* Z80-PIO Interface */

READ8_MEMBER( c80_state::pio1_pa_r )
{
	/*

	    bit     description

	    PA0     keyboard row 0 input
	    PA1     keyboard row 1 input
	    PA2     keyboard row 2 input
	    PA3
	    PA4     _BSTB input
	    PA5     display enable output (0=enabled, 1=disabled)
	    PA6     tape output
	    PA7     tape input

	*/

	uint8_t data = !m_pio1_brdy << 4 | 0x07;

	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(m_keylatch, i))
		{
			if (!BIT(m_row0->read(), i)) data &= ~0x01;
			if (!BIT(m_row1->read(), i)) data &= ~0x02;
			if (!BIT(m_row2->read(), i)) data &= ~0x04;
		}
	}

	data |= (m_cassette->input() < +0.0) << 7;

	return data;
}

WRITE8_MEMBER( c80_state::pio1_pa_w )
{
	/*

	    bit     description

	    PA0     keyboard row 0 input
	    PA1     keyboard row 1 input
	    PA2     keyboard row 2 input
	    PA3
	    PA4     _BSTB input
	    PA5     display enable output (0=enabled, 1=disabled)
	    PA6     tape output
	    PA7     tape input

	*/

	m_pio1_a5 = BIT(data, 5);

	if (!BIT(data, 5))
	{
		m_digit = 0;
	}

	m_cassette->output(BIT(data, 6) ? +1.0 : -1.0);
}

WRITE8_MEMBER( c80_state::pio1_pb_w )
{
	/*

	    bit     description

	    PB0     VQD30 segment A
	    PB1     VQD30 segment B
	    PB2     VQD30 segment C
	    PB3     VQD30 segment D
	    PB4     VQD30 segment E
	    PB5     VQD30 segment F
	    PB6     VQD30 segment G
	    PB7     VQD30 segment P

	*/

	if (!m_pio1_a5)
	{
		m_digits[m_digit] = data;
	}

	m_keylatch = data;
}

WRITE_LINE_MEMBER( c80_state::pio1_brdy_w )
{
	m_pio1_brdy = state;

	if (state)
	{
		if (!m_pio1_a5)
		{
			m_digit++;
		}

		m_pio1->strobe_b(1);
		m_pio1->strobe_b(0);
	}
}

/* Z80 Daisy Chain */

static const z80_daisy_config c80_daisy_chain[] =
{
	{ Z80PIO1_TAG },
	{ Z80PIO2_TAG },
	{ nullptr }
};

/* Machine Initialization */

void c80_state::machine_start()
{
	m_digits.resolve();
	/* register for state saving */
	save_item(NAME(m_keylatch));
	save_item(NAME(m_digit));
	save_item(NAME(m_pio1_a5));
	save_item(NAME(m_pio1_brdy));
}

/* Machine Driver */

MACHINE_CONFIG_START(c80_state::c80)
	/* basic machine hardware */
	Z80(config, m_maincpu, 2500000); /* U880D */
	m_maincpu->set_addrmap(AS_PROGRAM, &c80_state::c80_mem);
	m_maincpu->set_addrmap(AS_IO, &c80_state::c80_io);
	m_maincpu->set_daisy_config(c80_daisy_chain);

	/* video hardware */
	config.set_default_layout(layout_c80);

	/* devices */
	Z80PIO(config, m_pio1, 2500000);
	m_pio1->out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
	m_pio1->in_pa_callback().set(FUNC(c80_state::pio1_pa_r));
	m_pio1->out_pa_callback().set(FUNC(c80_state::pio1_pa_w));
	m_pio1->out_pb_callback().set(FUNC(c80_state::pio1_pb_w));
	m_pio1->out_brdy_callback().set(FUNC(c80_state::pio1_brdy_w));

	z80pio_device& pio2(Z80PIO(config, Z80PIO2_TAG, XTAL(2500000)));
	pio2.out_int_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);

	MCFG_CASSETTE_ADD("cassette")
	MCFG_CASSETTE_DEFAULT_STATE(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED )

	SPEAKER(config, "mono").front_center();
	WAVE(config, "wave", "cassette").add_route(ALL_OUTPUTS, "mono", 0.25);

	/* internal ram */
	RAM(config, RAM_TAG).set_default_size("1K");
MACHINE_CONFIG_END

/* ROMs */

ROM_START( c80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "c80.d3", 0x0000, 0x0400, CRC(ad2b3296) SHA1(14f72cb73a4068b7a5d763cc0e254639c251ce2e) )
ROM_END


/* System Drivers */

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT  CLASS      INIT        COMPANY          FULLNAME  FLAGS */
COMP( 1986, c80,  0,      0,      c80,     c80,   c80_state, empty_init, "Joachim Czepa", "C-80",   MACHINE_SUPPORTS_SAVE )
