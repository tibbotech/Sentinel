#!/usr/bin/env python
#
import os, sys
import struct
import string
import xml.parsers.expat
import argparse
import logging
import re

parser = None
logger = logging.getLogger("scomp")
sensors = [None for i in range(256)]
cur_sensor = None
in_init = False

LOG_FMT = "SC: {message}s"


BUSF_I2C            = 0x01
BUSF_1W             = 0x02
BUSF_OW             = 0x04
BUSF_SW             = 0x08
BUSF_RS485          = 0x10
BUSF_RS232          = 0x20
BUSF_DC             = 0x40
BUSF_TIBBO_GENUINE  = 0x80


STR_ENCODING        = "utf-8"

# These commands are used in I/O program
#
CMD_WRITE           = ord('W') # 0x00
CMD_READ            = ord('R') # 0x01
CMD_DELAY           = ord('D') # 0x02
CMD_WRITE_READ      = ord('C') # 0x03      # Combined write-than-read in one bus transaction
CMD_END             = ord('.') # 0xFF


TYPE_INIT           = "INIT"
TYPE_READ           = "READ"


VAL_TYPES = {
    "none": 0,
    "temperature": 1,
    "humidity": 2,
    "temperature_and_humidity": 3,
    "flood": 4,
    "fire": 5,
    "smoke": 6,
    "movement": 7,
    "acdc_current": 8,
    "shock": 9,
    "reed_switch": 10,
    "switch": 11,
    "pressure": 12,
    "light": 13,
    "acceleration_three_axis": 14,
    "acceleration_six_axis": 15,
    "bit": 16,
    "byte": 17,
    "word": 18,
    "dword": 19,
    "gas_presence_co": 20,
    "gas_presence_co2": 21,
    "gas_presence_methane": 22,
    "alcohol": 23
}


_stack = list()


def set_state(name, newstate):
    global _stack, parser
    #print("%s%s" % (" " * (len(_stack)*2), name))
    _stack.append(parser.StartElementHandler)
    parser.StartElementHandler = newstate
    
    
def pop_state(dummy=None):
    global _stack, parser
    
    tmp = None if len(_stack) == 0 else _stack.pop()
    if tmp is not None:
        parser.StartElementHandler = tmp
        parser.CharacterDataHandler = None
        #print("%send %s" % (" " * (len(_stack)*2), dummy))
    

def none_handler(elem, attrs):
    pass


def skip(elem, attrs = None):
    set_state(elem, skip)


# this is limited subset of P-code instructions
#
instr_opcodes = {
    "nop":      { "ops": 0, "code": 0x00 },
    "copysign": { "ops": 2, "code": 0x01 },
    "mov":      { "ops": 2, "code": 0x03 },
    "add":      { "ops": 2, "code": 0x04 },
    "and":      { "ops": 2, "code": 0x05 },
    "or":       { "ops": 2, "code": 0x06 },
    "mul":      { "ops": 2, "code": 0x07 },
    "shl":      { "ops": 2, "code": 0x08 },
    "swap":     { "ops": 1, "code": 0x09 },
    "swapw":    { "ops": 1, "code": 0x0A },
    "shr":      { "ops": 2, "code": 0x0B },
    "float":    { "ops": 1, "code": 0x0C },
    "int":      { "ops": 1, "code": 0x0D },
    "div":      { "ops": 2, "code": 0x0E },
    "ret":      { "ops": 0, "code": 0x0F }
}


reg_regexp = re.compile('(?P<opcode>[a-z]+)(\s+(?P<op1>[rc][0-7])(\s*\,\s*(?P<op2>.*))?)?', re.I)
arg_regexp = re.compile('(?P<hex>0?x[0-9a-f]+)|(?P<dec>[+-]?[0-9]+(.[0-9]+)?)|(?P<reg>[rc][0-7])|(?P<const>c[0-7])|(((?P<dt>(byte\s+|word\s+|dword\s+))?(?P<data>data)\[(?P<ind>[0-9]+))\])', re.I)


def num(s):
    try:
        return int(s)
    except ValueError:
        return float(s) 


def parse_op(opc, s, narg):
    """Todo: get rid of hardcoded constants
    """
    g = arg_regexp.match(s)
    if g is None:
        print("Invalid operand %d: \"%s\"" % (narg, s))
        return None
    
    if g.group("reg") is not None:
        r = g.group("reg")
        rn = ord(r[1]) - ord('0')
        if r[0] == 'c':
            if narg == 1:
                return (rn+8,)
            else:
                return (0xE, bytearray([rn]))
        else:
            return (rn,)

    if narg == 1:
        print("Error: first arg must be register: %s" % s)
        return None
    
    if g.group("const") is not None:
        c = g.group("const")
        rn = ord(s[1]) - ord('0')
        return (0x0E, bytearray([rn]))
    
    elif g.group("dec") is not None:
        n = num(g.group("dec"))
        if opc in [0xB, 0x8]:
            return (0x8, bytearray([n])) # index!
        if isinstance(n, float):
            return (0xD, struct.pack("f", n))
        return (0x0F, n.to_bytes(4, 'little'))
    elif g.group("hex") is not None:
        n = int(g.group("hex"), 16)
        return (0x0F, n.to_bytes(4, 'little'))
    
    elif g.group("data") is not None:
        dt = g.group("dt").strip()
        n = int(g.group("ind"))
        if dt == "byte":
            return (0x0A, bytearray([n]))
        elif dt == "word":
            return (0x0B, bytearray([n]))
        elif dt == "dword" or dt == "":
            return (0x0C, bytearray([n]))

    return None

        
def asm(s):
    """Parse a single assembler instruction
    and return opcode bytes"""
    
    g = reg_regexp.match(s.strip().lower())
    if g is None:
        print("Not a valid instruction \"%s\"" % s)
        return None

    res = bytearray()
    
    info = instr_opcodes.get(g.group("opcode"), None)
    if info is None:
        print("Unknown opcode \"%s\"" % s)
        return None

    no = info["ops"]
    opc = info["code"]
    res.append(opc)

    exp = g.group("op1")
    if no == 0:
        if exp is not None:
            print("Excess operand \"%s\"" % exp)
            return None
        return res
    elif no == 1:
        if exp is None:
            print("Missing operand #1")
            return None
        if g.group("op2") is not None:
            print("Excess operand \"%s\"" % g.group("op2"))
            return None
    elif no == 2:
        if g.group("op2") is None:
            print("Missing operand #2: \"%s\"" %s)
            return None

    op1 = parse_op(opc, exp, 1)
    op2 = (0)
    if no == 2:
        exp = g.group("op2")
        op2 = parse_op(opc, exp, 2)
        res.append((op1[0]<<4) | op2[0])
        #print("%s , %s = %s" % (op1[0], op2[0], hex((op1[0]<<4) | op2[0])))
        if(len(op2)>1):
            res += op2[1]
    else:
        res.append(op1[0]<<4)

    return res


class bitfield:
    __slots__ = ["start", "size", "scale", "offset", "byteorder", "code"]
    
    def __init__(self):
        self.start = 0
        self.size = 1
        self.scale = 1
        self.offset = 0
        self.byteorder = 'lsb-first'
        self.code = bytes()
        
    def compile(self):
        if len(self.code) == 0:
            res = bytearray()
            
            if self.size <= 8:
                res += asm("mov r0, byte data[0]")
            elif self.size <= 16:
                res += asm("mov r0, word data[0]")
            else:
                res += asm("mov r0, dword data[0]")
                
            if self.byteorder in ['hi-endian', 'msb-first']:
                res += asm("swap r0")
            
            if self.start != 0:
                res += asm("shr r0, " + str(self.start))
                
            res += asm("and r0, " + hex((1<<self.size)-1))
            
            res += asm("float r0")
            
            if self.scale != 1:
                res += asm("mul r0, " + str(self.scale))
                
            if self.offset != 0:
                res += asm("add r0, " + str(self.offset))
       
            res += asm("ret")         
            self.code = res
    
    def reset(self):
        self.code = bytes()

    def sizeof(self):
        self.compile()
        return len(self.code)+1
    
    def write(self, fp):
        self.compile()
        l = len(self.code)
        if l > 0:
            fp.write(struct.pack('B', l))
            fp.write(self.code)
            #print(self.code)
            
    def __repr__(self):
        return "<bitfield start=%d size=%d scale=%f offset=%d>" % (self.start, self.size, self.scale, self.offset)
    

class code:
    __slots__ = ["byteorder", "code", "source", "old_handler"]
    
    def __init__(self):
        self.byteorder = 'lsb-first'
        self.code = bytes()
        self.source = "";
        self.old_handler = None
        
    def cdata_handler(self, code):
        self.source += code
    
    def end_element(self, elem):
        parser.EndElementHandler = self.old_handler
        pop_state()

        for line in self.source.split("\n"):
            instr = line.split(";")[0].strip()
            if len(instr) != 0:
                c = asm(instr)
                self.code += c
    
    def reset(self):
        self.code = bytes()

    def sizeof(self):
        return len(self.code)+1
    
    def write(self, fp):
        l = len(self.code)
        if l > 0:
            fp.write(struct.pack('B', l))
            fp.write(self.code)
            
    def __repr__(self):
        return "<code %d bytes>" % len(self.code)
    
    
class param:
    __slots__ = ["type", "byteorder", "name", "fields"]
    
    def __init__(self):
        self.type = 0
        self.byteorder = 0
        self.name = ""
        self.fields = list()

    def element_handler(self, elem, attrs):
        el = elem.lower()
        if el == "bitfield":
            bf = bitfield()
            bf.start = int(attrs.get("start", 0))
            bf.size = int(attrs.get("size", 1))
            bf.scale = float(attrs.get("scale", 1.0))
            bf.offset = float(attrs.get("offset", 0.0))
            bf.byteorder = self.byteorder
            self.fields.append(bf)
            skip(el)
        elif el == "code":
            c = code()
            c.byteorder = self.byteorder
            c.old_handler = parser.EndElementHandler
            self.fields.append(c)
            parser.CharacterDataHandler = c.cdata_handler
            parser.EndElementHandler = c.end_element
            set_state(el, None)

    def sizeof(self):
        res = 0
        for f in self.fields:
            res += f.sizeof()
        return res
    
    def write(self, fp):
        #print(self.fields)
        fp.write(struct.pack('B', len(self.fields)))
        for f in self.fields:
            f.write(fp)
    

class program:
    __slots__ = ["data", "type", "params", "cmd"]

    def __init__(self, t = TYPE_READ):
        self.params = list()
        self.data = bytearray()
        self.type = t
        self.cmd = bytearray()
        
    def add_write(self, ba):
        self.data.append(CMD_WRITE)
        self.data.append(len(ba))
        self.data += ba
    
    def add_read(self, ba):
        self.data.append(CMD_READ)
        self.data.append(1)
        self.data.append(ba)

    def add_delay(self, ms):
        self.data.append(CMD_DELAY)
        self.data.append(int(ms) & 0xFF)
        
    def write(self, fp):
        fp.write(struct.pack("H", len(self.data)))
        fp.write(self.data)

        cb = struct.calcsize("B%dB" % len(self.params))
        for p in self.params:
            cb += p.sizeof()
            
        fp.write(struct.pack('HB', cb, len(self.params)))
        for p in self.params:
            p.write(fp)

    def reply_handler(self, elem, attrs):
        el = elem.lower()
        if el == "param":
            p = param()
            p.type = attrs.get("type", "float")
            p.byteorder = attrs.get("byteorder", "lsb-first")
            p.name = attrs.get("name", "")
            self.params.append(p)
            set_state("param", p.element_handler)
        
    def elem_start_handler(self, elem, attrs):
        el = elem.lower()
        if el == "command":
            self.cmd = bytearray()
            parser.CharacterDataHandler = self.cdata_handler
            parser.EndElementHandler = self.compile_command
            set_state(el, None)
        elif el == "reply":
            l = attrs.get("length", "1")
            if l.isnumeric():
                self.add_read(int(l))
                set_state(el, self.reply_handler)

    def compile_command(self, elem):
        parser.EndElementHandler = pop_state
        pop_state()
        self.add_write(self.cmd)
    
    def cdata_handler(self, data):
        data = data.strip()
        try:
            d = bytearray().fromhex(data)
        except ValueError:
            print("Error: Not a valid hex number: '%s'" % data)
        else:
            self.cmd += d
            

class sensor:
    __slots__ = ["sid", "name", "chipset", "valuetype", "buslist", "read", "init", "fileoffs", "tibno"]

    def __init__(self):
        self.sid = 0
        self.name = ""
        self.chipset = ""
        self.valuetype = 0
        self.buslist = list()
        self.read = program(TYPE_READ)
        self.init = program(TYPE_INIT)
        self.fileoffs = -1
        self.tibno = 0
        
    def __str__(self):
        return "<sensor id={0} name=\"{1}\" buslist=\"{2}\" chipset=\"{3}\">".format(
            self.sid, self.name, ", ".join(self.buslist), self.chipset)
    
    def write(self, fp):
        self.fileoffs = fp.tell()
        
        bs = struct.calcsize("B")
        ws = struct.calcsize("H")
        
        rl = bs + bs*len(self.name) + bs + bs*len(self.chipset)


#     1      1        1        1           2          1        Len        1        Len                2                              2
# +------+-------+---------+-------+-------+-------+-----+--- - - - ---+-----+-- - - - - - ---+-------+-------+--- - - - - --+-------+-------+--- - - - - ---+
# |  ID  | Flags | TibboID |ValType|Length of chunk| Len | Name string | Len | Chipset string |Length of chunk| READ program |Length of chunk| INIT program  |
# +------+-------+---------+-------+-------+-------+-----+--- - - - ---+-----+-- - - - - - ---+-------+-------+-- - - - - ---+-------+-------+--- - - - - ---+
#                                                   \________________________________________/                 \____________/                 \______________/
#                                                                      chunk                                       chunk                           chunk
#
        print("[DEVID %d]" % self.sid)
        fp.write(struct.pack("BBBBH", self.sid, self.busflags(), self.tibno, self.valuetype, rl))
        
        tmp = bytes(self.name, STR_ENCODING)
        fmt = "B{}s".format(len(tmp))
        fp.write(struct.pack(fmt, len(tmp), tmp))
        
        tmp = bytes(self.chipset, STR_ENCODING)
        fmt = "B{}s".format(len(tmp))
        fp.write(struct.pack(fmt, len(tmp), tmp))
        
        self.init.write(fp)
        self.read.write(fp)
        
        return self.fileoffs
    
    def busflags(self):
        bf = 0
        if("i2c" in self.buslist):
            bf |= BUSF_I2C
        if "1-wire" in self.buslist:
            bf |= BUSF_1W
        if "s-wire" in self.buslist:
            bf |= BUSF_SW
        if "dc" in self.buslist:
            bf |= BUSF_DC
        if "rs-485" in self.buslist:
            bf |= BUSF_RS485
        if self.tibno > 0:
            bf |= BUSF_TIBBO_GENUINE
        return bf
    
    def set_name(self, data):
        self.name = data.strip()
        
    def set_chipset(self, data):
        self.chipset = data.strip()

    def bus_set_name(self, data):
        self.buslist.append(data.strip())
        
    def buslist_handler(self, elem, attrs):
        global parser
        if elem.lower() == "bus":
            parser.CharacterDataHandler = self.bus_set_name
            set_state(elem, None)
            
    def inner_tags_handler(self, elem, attrs):
        global parser
        
        el = elem.lower();
        if el == "name":
            parser.CharacterDataHandler = self.set_name
            set_state(el, None)
        elif el == "chipset":
            parser.CharacterDataHandler = self.set_chipset
            set_state(el, None)
        elif el == "buslist":
            set_state(el, self.buslist_handler)
        elif el == "init":
            #print("CMD_INIT_START")
            set_state(el, self.init.elem_start_handler)
        elif el == "read":
            #print("CMD_READ_START")
            set_state(el, self.read.elem_start_handler)
    
        
def sensor_handler(elem, attrs):
    global parser, cur_sensor, sensors
    
    el = elem.lower();
    if el == "sensor":
        sids = attrs.get("id", "0")
        if sids.isnumeric():
            sid = int(sids)
        else:
            sid = 0
            
        types = attrs.get("valuetype", "byte")
        vt = VAL_TYPES.get(types, -1)
        if vt == -1:
            print("Wrong 'valuetype' specified for sensor %d" % sids)
            pass
        
        cur_sensor = sensor()
        cur_sensor.sid = sid
        cur_sensor.valuetype = vt
        sensors[sid] = cur_sensor
        
        set_state(el, cur_sensor.inner_tags_handler)


def sensors_handler(elem, attrs):
    global parser
    if elem.lower() == "sensors":
        set_state(elem, sensor_handler)


def main(argv):
    global parser, sensors, logger
    
    #logger.basicConfig(format=LOG_FMT)
    
    ap = argparse.ArgumentParser(prog="scomp");
    
    ap.add_argument("-o", "--output", nargs=1, help="Set output file name and location");
    ap.add_argument("infile", help="Input XML file with sensor definitions");
    
    ns = ap.parse_args(argv);

    args = vars(ns);
    if ("h" in args) or len(ns.infile) == 0:
        ap.print_help()
        return 0
    
    with open(ns.infile, 'rb') as f:
        parser = xml.parsers.expat.ParserCreate()
        parser.EndElementHandler = pop_state
        set_state("@@initial@@", sensors_handler)
    
        try:
            parser.ParseFile(f);
        except xml.parsers.expat.ExpatError as err:
            print("Error:", xml.parser.expat.errors.messages[err.code]);

    outfs = args.get("output", "sensors.bin")[0];
    try:
        outf = open(outfs, "w+b")
    except:
        print("Cannot write %s" % outfs)
        return -1
    
    outf.write(struct.pack("cccc", b'S', b'N', b'D', b'B'))
    
    for i in range(256):
        outf.write(struct.pack("H", 0))
    
    nr = 0
    for sens in sensors:
        if sens is not None:
            nr += 1
            sens.write(outf)
        
    for sens in sensors:
        if sens is not None:
            outf.seek(4 + sens.sid*2, os.SEEK_SET)
            outf.write(struct.pack("H", sens.fileoffs))
            
    outf.seek(0, os.SEEK_END)
    
    print("Status: %s" % "Success")
    print("Records: %d" % nr)
    print("Filesize: %d" % outf.tell())

    outf.close()
    return 0

    
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]));
    
