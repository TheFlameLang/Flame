
#include <cstdlib>
#include <iostream>
#include <vector>

#include <xbyak/xbyak.h>

#define _NEKOVM_VERSION_ "1.1.0"

typedef unsigned long long u64;
typedef long long i64;
typedef double f64;
typedef unsigned char u8;

enum IS_ {
  LOAD = 1,
  ADD,
  SUB,
  MUL,
  DIV,
  MULS,
  DIVS,
  INC,
  DEC,
  XOR,
  AND,
  OR,
  SHR,
  SHL,
  NOT,
  CMP,
  JMP,
  JE,
  JNE,
  JZ,
  JNZ,
  JST,
  JBT,
  JSE,
  JBE,
  COPY,
  PUSH,
  POP,
  VMCALL,
  CALL,
  RET,
  HALT = 255
};

class NekoVM : public Xbyak::CodeGenerator {
protected:
  u64 u8s2u64(u8 arr[], u64 i) {
    u64 result = 0;
    for (int j = 0; j < 8; j++) {
      result |= (u64)arr[i + j] << (j * 8);
    }
    return result;
  }

  void vmsyscall(u8 a, u8 b, u8 c) {
    switch (a) {
    case 0: {
      return;
    }
    case 1: {
      std::cout << this->r[b] << std::endl;
      break;
    }
    case 2: {
      std::cin >> this->r[b];
      break;
    }
    case 3: {
      exit(b);
    }
      return;
    }
  }

public:
  i64 r[32];
  f64 fr[16];
  std::vector<u8> memory;
  std::vector<u64> stack;
  u64 *call_stack = new u64[4096];
  u64 call_sp;
  std::vector<Xbyak::Label> labels;
  u64 prog_size = 0;
  NekoVM(const u64 mem_size) {
    memory.resize(mem_size);
    for (auto &x : r) {
      x = 0;
    }
    for (u64 z = 0; z < mem_size; z++) {
      memory[z] = 0;
    }
    call_sp = 0;
  }

  void load_program(const std::vector<u8> &prog) {
    for (u64 i = 0; i < prog.size(); i++) {
      if (i >= memory.size()) {
        return;
      }
      memory[i] = prog[i];
    }
    prog_size = prog.size();
    labels.resize(prog_size);
  }

  

  void jit() {
    push(r12);
    push(r13);
    xor_(r12, r12);
    mov(r13, rsi);
    for (u64 ip = 0; ip < prog_size;) {
      L(labels[ip]);
      u8 op = memory[ip];
      ip++;
      switch (op) {
      case LOAD: {
        u8 reg = memory[ip++];

        u64 value = u8s2u64(memory.data(), ip);
        ip += 8;
        mov(rax, value);
        mov(qword[rdi + reg * 8], rax);

        break;
      }

      case ADD: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        add(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case SUB: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        sub(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);
        break;
      }

      case MUL: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        mul(qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);
        break;
      }

      case DIV: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];
        mov(rcx, qword[rdi + b * 8]);
        cmp(rcx, 0);
        jz("skip_div");
        xor_(rdx, rdx);
        mov(rax, qword[rdi + a * 8]);
        div(rcx);
        mov(qword[rdi + a * 8], rax);
        L("skip_div");
        break;
      }

      case MULS: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        imul(qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);
        break;
      }

      case DIVS: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];
        mov(rcx, qword[rdi + b * 8]);
        cmp(rcx, 0);
        jz("skip_divs");
        xor_(rdx, rdx);
        mov(rax, qword[rdi + a * 8]);
        idiv(rcx);
        mov(qword[rdi + a * 8], rax);
        L("skip_divs");
        break;
      }

      case INC: {
        u8 a = memory[ip++];
        mov(rax, qword[rdi + a * 8]);
        inc(rax);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case DEC: {
        u8 a = memory[ip++];
        mov(rax, qword[rdi + a * 8]);
        dec(rax);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case XOR: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        xor_(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case AND: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        and_(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case OR: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        mov(rax, qword[rdi + a * 8]);
        or_(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);

        break;
      }

      case SHR: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        shr(qword[rdi + a * 8], b);
        break;
      }

      case SHL: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];

        shl(qword[rdi + a * 8], b);
        break;
      }

      case NOT: {
        u8 a = memory[ip++];

        not_(qword[rdi + a * 8]);
        break;
      }

      case CMP: {
        u64 a = memory[ip++];
        u64 b = memory[ip++];
        mov(rax, qword[rdi + a * 8]);
        cmp(rax, qword[rdi + b * 8]);
        break;
      }

      case JMP: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jmp(labels[addr]);
        break;
      }

      case JE: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        je(labels[addr]);
        break;
      }

      case JNE: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jne(labels[addr]);
        break;
      }

      case JZ: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jz(labels[addr]);
        break;
      }

      case JNZ: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jnz(labels[addr]);
        break;
      }

      case JST: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jl(labels[addr]);
        break;
      }

      case JBT: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jb(labels[addr]);
        break;
      }

      case JSE: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jle(labels[addr]);
        break;
      }

      case JBE: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        jbe(labels[addr]);
        break;
      }

      case COPY: {
        u8 a = memory[ip++];
        u8 b = memory[ip++];
        mov(rax, qword[rdi + b * 8]);
        mov(qword[rdi + a * 8], rax);
        break;
      }

      case PUSH: {
        u64 a = memory[ip++];
        push(qword[rdi + a * 8]);
        break;
      }

      case POP: {
        u8 a = memory[ip++];
        pop(rax);
        mov(qword[rdi + a * 8], rax);
        break;
      }

      /*
      case VMCALL: {
        vmsyscall(memory[ip], memory[ip + 1], memory[ip + 2]);
        ip += 3;
        break;
      }
      */

      case CALL: {
        u64 addr = u8s2u64(memory.data(), ip);
        ip += 8;
        Xbyak::Label ret_l;
        mov(rax, ret_l);
        mov(qword[r13 + r12 * 8], rax);
        inc(r12);
        jmp(labels[addr]);
        L(ret_l);
        break;
      }

      case RET: {
        dec(r12);
        mov(rax, qword[r13 + r12 * 8]);
        jmp(rax);
        break;
      }

      case HALT: {
        pop(r13);
        pop(r12);
        ret();
        return;
      }
      default:
        ip++;
        break;
      }
    }
  }

  ~NekoVM() { delete[] call_stack; }
};

int main(int argc, char* argv[]) {
  /*
  bool dump = false;
  for(int i=0;i<argc;i++) {
    if(std::strcmp(argv[i], "dump")==0) {
      dump = true;
    }
    if(std::strcmp(argv[i], "compile")==0) {
      if(argc>i+1) {
        std::fstream file(argv[i+1]);
        std::vector<u8> prog;
        char c;
        while(file.get(c)) prog.push_back(c);
        NekoVM vm(prog.size());
        vm.load_program(prog);
        vm.jit();
        vm.ready();
        auto fn = vm.getCode<void (*)(u64 *, u64 *)>();
        if(dump) vm.dump();
        fn(vm.r, vm.call_stack);
        file.close();
        return 0;
      }
    } 
  }
  */
  std::vector<u8> prog = {
    LOAD, 0, 0x05, 0xF5 , 0xE1 , 0x00 , 0x00, 0, 0, 0,
    INC, 1,
    CMP, 0, 1,
    JNE, 10, 0,0,0,0,0,0,0,
    HALT
  };
  NekoVM vm(32);
  vm.load_program(prog);
  vm.jit();
  vm.ready();
  vm.dump();
  auto fn = vm.getCode<void (*)(i64 *, u64 *)>();
  fn(vm.r, vm.call_stack);
  std::cout << "After: " << vm.r[1] << "\n";
  return 0;
}