#!/usr/bin/env python2
#vim: set ts=2 sts=2 sw=2 et si tw=80:
import re
import sys
import subprocess
import string
import ipdb

#Build a CFG for a function based on objdump -d output
#usage: objdump_to_cfg.py "objdump -d output" function_name

#jumps from
#http://blog.fairchild.dk/2013/06/automagical-asm-control-flow-arrow-annotation/
jumps = { #define jumps, synomyms on same line
'ja':'if above', 'jnbe':'if not below or equal',
'jae':'if above or equal','jnb':'if not below','jnc':'if not carry',
'jb':'if below', 'jnae':'if not above or equal', 'jc':'if carry',
'jbe':'if below or equal', 'jna':'if not above',
'jcxz':'if cx register is 0', 'jecxz':'if cx register is 0',
'je':'if equal', 'jz':'if zero',
'jg':'if greater', 'jnle':'if not less or equal',
'jge':'if greater or equal',
'jl':'if less', 'jnge':'if not greater or equal',
'jle':'if less or equal', 'jnl':'if not less',
'jne':'if not equal', 'jnz':'if not zero',
'jng':'if not greater',
'jno':'if not overflow',
'jnp':'if not parity', 'jpo':'if parity odd',
'jns':'if not sign',
'jo':'if overflow',
'jp':'if parity', 'jpe':'if parity even',
'js':'if sign'}

jumps_uncond = { 
'jmp':'unconditional',
'jmpq': 'unconditional qword'}

calls = {'call': 'call', 'callq':'call qword'} 
rets = {'ret':'return', 'retq':'return qword', 'repz ret':'repz ret'}

#below regexes based on those from Lok
symbol_re = re.compile("^([a-fA-F0-9]+) <([\.\w]+)>:\s*$") 
symbol_plt_re = re.compile("^([a-fA-F0-9]+) <([@\w]+)>:\s*$")

#Offset to add to all addresses. this is 32bit ELF w/o ASLR
#offset=0x80000000
offset=0

def main(dump_file, root_name):
  #CFG: key is source address, values are all targets
  #     note that source=0 means root and source=-1 means we came from an indirect
  CFG={}
  
  root=0 #The root of our CFG
  
  #Store the objdump in memory, list of tuples (address, asm)
  #Went with this over a dict since we should be iterating more than searching
  #so having it sorted by address just makes sense
  objdump=[]
  indirects=[]
  blockqueue=[]
  paths_visited=[]

  print "#address;[target1, target2, ...]" #our output format
  print "#0 == root, -1 == indirect" #right now we only use indirects as sources
  
  with open(dump_file) as f:
    for line in f:
      #First find out function of interest
      m=symbol_re.match(line)
      if(m):
        if(m.group(2) == root_name):
          print(m.group(1), m.group(2))
          root=offset+int(m.group(1),16)

      #Example tab-delimited output from objdump:
      #ADDRESS    INSTRUCTION           ASCII
      #c1000000:  8b 0d 80 16 5d 01     mov    0x15d1680,%ecx
      fields = string.split(line.rstrip(),'\t')
      
      #Symbols have 2 fields, but their actual first instruction will have 3
      if(len(fields)<3):
        continue
      #objdump -d is a linear disassembler, so we every we get will already be
      #sorted by address. If you use -D or something, you may need to re-sort

      #Our in-memory representation of objdump output is an list of tuples of 
      #the form (address, instruction)
      #Note that we store the address in int form for searching
      objdump.append((offset+int(fields[0].replace(":",""),16), fields[2]))

  if(root==0):
    print "Could not find root function for CFG: %s" % root_name

  #process BBs until we get a return to 0, used to be recursive, but python
  #doesn't do tail recursion so this should be better?
  blockqueue.append((0, root, [0])) #queue is technically a stack, but whatever
  while(len(blockqueue)>0):
    block=blockqueue.pop()
    iterate_bb(block[0], block[1], block[2], blockqueue, CFG, paths_visited, objdump, indirects)
  print_CFG(CFG)
  print("Indirect calls/jumps: %d"%len(indirects))
  print("%s"%array_to_hex(indirects))

####END MAIN BEGIN FUNCTION DEFS#####
def array_to_hex(array):
  return string.join('0x%x' % i for i in array)

def print_CFG(CFG):
  for(key, value) in CFG.iteritems():
    print("0x%x: %s"%(key, array_to_hex(value)))

def get_objdump_index(address, objdump):
  #Binary search in our array to find our target
  item = 0
  first = 0
  last = len(objdump)-1

  while first<=last:
    mid = (first+last)/2
    item = objdump[mid][0]
    if(item==address):
      return mid
    else:
      if(address < item):
        last = mid-1
      else:
        first = mid+1
  raise Exception("couldn't find address {}. Last searched was: {}".format(address, item))

def print_instr(instr):
  #Useful for debugging if you want to see what's going on
  print("0x%x: %s"%(objdump[instr][0],objdump[instr][1]))

def count_indirect(source_addr, indirects):
  indirects.append(source_addr)

def iterate_bb(source, blockaddr, callstack, blockqueue, CFG, paths_visited, objdump, indirects):
  #The definition of basic block is not the compiler definition!
  #That is, we can jump inside the middle of blocks
  #We also sling around a callstack (passed by value) for proper return tracking

  #If you want to trace exactly what is happening, uncomment this:
  print("current: src: 0x%x, block: 0x%x, stack: %s"%(source,blockaddr,
        array_to_hex(callstack)))
  print("queue: %s"%map(lambda x: "(source: 0x%x, target: 0x%x, stack: %s)"%\
                          (x[0], x[1], array_to_hex(x[2])), blockqueue))
  if(blockaddr==0 or blockaddr==-1):
    #Stop conditions: we return to root or an indirect (somehow)
    return

  if(source not in CFG):
    #First outgoing edge
    CFG[source]=[]
  if(blockaddr not in CFG[source]):
    CFG[source].append(blockaddr)

  if((source, blockaddr, callstack[:]) in paths_visited):
    #Poor man's loop detection
    return
  paths_visited.append((source, blockaddr, callstack[:]))

  skip_next_jump = False
  #Now, step through until we hit a jump, call, or ret
  for i in range(get_objdump_index(blockaddr, objdump),len(objdump)):
    target_hex=0
    split = string.split(objdump[i][1])
    if split[0] != 'repz':
        instr=split[0]
        #print_instr(i)
        if(len(split)>=2):
          target = split[1]
          try:
            target_hex = offset+int(target,16)	
          except:
            #already an int or an indirect (which won't be used)
            target_hex = target
    else:
        instr = split[1]
        target = []
    print 'instr: {}'.format(instr)
    print 'target: {}'.format(target)
    print 'target_hex: {}'.format(target_hex)

    #Check for GCC stack protector. That next conditional jump will check for
    #fail, so let's skip it
    if("xor" in instr and "gs:0x14" in target):
      skip_next_jump = True
      continue
    
    #Look for conditional jumps
    if(instr in jumps):
      print 'conditional'
      if skip_next_jump:
        print 'skipped'
        #To avoid following stack protector failure paths...
        #Only process not taken branch
        blockqueue.append((blockaddr, objdump[i+1][0], callstack[:]))
        return
      if '*' in target:
        print 'indirect'
        #Indirect, track it and abandon all hope
        count_indirect(objdump[i][0], indirects)
        return
      else:
        print 'direct'
        #Handle jump not taken
        blockqueue.append((blockaddr, objdump[i+1][0], callstack[:]))
        #Handle jump taken
        blockqueue.append((blockaddr, target_hex, callstack[:]))
        return

    #Look for unconditional jumps
    if(instr in jumps_uncond):
      print 'unconditional'
      if skip_next_jump:
        print 'skipped'
        #To avoid following stack protector failure paths...
        #Only process not taken branch
        blockqueue.append((blockaddr, objdump[i+1][0], callstack[:]))
        return
      if '*' in target:
        print 'indirect'
        #Indirect, track it and abandon all hope
        count_indirect(objdump[i][0], indirects)
        return
      else:
        print 'direct'
        blockqueue.append((blockaddr, target_hex, callstack[:]))
        return 

    #Look for calls 
    if(instr in calls):
      print 'call'
      #This is a hackish heuristic to get around calls that never return:
      #which cause a huge problem for our CFG since we'll just skip over them
      #and act as if they had returned
      #If we see a push %ebp or sub X,%esp, then we've hit the next function.
      #Seeing that before a ret or unconditional jump means this call was never 
      #expected to return so we bail
      for j in range(i,len(objdump)):
        split_call = string.split(objdump[j][1])
        instr_call = split_call[0]
        target_call = ""
        if(len(split_call)>=2):
          target_call = split_call[1]
        
        #Either one of these conditions represents a new function to us...
        if(("sub" in instr_call and "esp" in target_call)):
          return
        if(("push" in instr_call and "ebp" in target_call)):
          return

        #This path will not go into the next function if we return
        if(instr_call in rets or instr_call in jumps_uncond):
          break

      #Indirect or in PLT, just skip over it as if we returned, but track its
      #source as coming from an indirect
      if '*' in target or 'plt' in split[2]:
        count_indirect(objdump[i][0], indirects)
        blockqueue.append((-1, objdump[i+1][0], callstack[:]))
        return 
      else: #for readability
        #Tried appending within a slice, but python got angry at me
        newstack=callstack[:]
        newstack.append(objdump[i+1][0])
        blockqueue.append((blockaddr, target_hex, newstack))
        return

    if(instr in rets):
      #Return, pop address off the stack
      print("Returning, adding to blockqueue: (0x%x, 0x%x, %s)"%\
            (blockaddr, callstack[-1], array_to_hex(callstack[:-1])))
      blockqueue.append((blockaddr, callstack[-1], callstack[:-1]))
      return

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
