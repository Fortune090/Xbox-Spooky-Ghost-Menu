typedef unsigned int u32; typedef unsigned short u16;
extern int guarded_call(void (*fn)(void));
extern int gc_handler_export(void *er, void *frame, void *ctx, void *disp);
static u32 tib[4];
static unsigned char ctx[0x240];
static u32 er[5];
static int ran;
u32 gb1, gs1, gd1;
static int sys3(int n, int a, int b, int c) { int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory"); return r; }
static void out(const char *s) { int n = 0; while (s[n]) n++; sys3(4, 1, (int)s, n); }
static void outn(u32 v) { char b[12]; int k = 0; char o[12]; int i = 0; do { b[k++] = (char)('0' + v % 10); v /= 10; } while (v); while (k) o[i++] = b[--k]; o[i] = 0; out(o); }
static u32 rd_fs0(void) { u32 v; __asm__ volatile("movl %%fs:0, %0" : "=r"(v)); return v; }
static void normal(void) { ran = 1; }
static void faulting(void)
{
	u32 rec = rd_fs0(), eip, esp, ebp;
	u16 cw = 0x027F;
	__asm__ volatile("fldcw %0; fld1; fld1" :: "m"(cw));
	er[0] = 0xC0000005u; er[1] = 0;
	if (gc_handler_export(er, (void *)rec, ctx, 0) != 0) { out("handler did not resume\n"); return; }
	eip = *(u32 *)(ctx + 0x224); esp = *(u32 *)(ctx + 0x230); ebp = *(u32 *)(ctx + 0x220);
	__asm__ volatile("movl %0, %%esp; movl %1, %%ebp; jmp *%2" :: "r"(esp), "r"(ebp), "r"(eip));
}
struct user_desc { u32 entry_number, base_addr, limit; u32 flags; };
void _start(void)
{
	struct user_desc d;
	u16 sel, cw0 = 0x037F, cw1, sw;
	u32 b0, s0, d0, b1, s1, d1;
	int r;
	d.entry_number = (u32)-1; d.base_addr = (u32)tib; d.limit = 0xfff; d.flags = 1 /* seg_32bit */ | (1 << 6) /* useable */;
	if (sys3(243, (int)&d, 0, 0) != 0) { out("set_thread_area failed\n"); sys3(1, 1, 0, 0); }
	sel = (u16)((d.entry_number << 3) | 3);
	__asm__ volatile("movw %0, %%fs" :: "r"(sel));
	tib[0] = 0x11223344u;
	__asm__ volatile("fninit; fldcw %0" :: "m"(cw0));
	r = guarded_call(normal);
	out("normal: returned "); outn((u32)r); out(", ran "); outn((u32)ran); out(", fs:0 restored "); out(rd_fs0() == 0x11223344u ? "yes\n" : "NO\n");
	{
		u32 fn = (u32)faulting, rr;
		__asm__ volatile(
			"pushl %%ebx; pushl %%esi; pushl %%edi\n\t"
			"movl $0x1111, %%ebx; movl $0x2222, %%esi; movl $0x3333, %%edi\n\t"
			"pushl %1; call guarded_call; addl $4, %%esp\n\t"
			"movl %%ebx, gb1; movl %%esi, gs1; movl %%edi, gd1\n\t"
			"popl %%edi; popl %%esi; popl %%ebx"
			: "=a"(rr) : "r"(fn) : "ecx", "edx", "memory");
		b1 = gb1; s1 = gs1; d1 = gd1;
		r = (int)rr; b0 = 0x1111; s0 = 0x2222; d0 = 0x3333;
	}
	__asm__ volatile("fnstcw %0; fnstsw %1" : "=m"(cw1), "=m"(sw));
	out("fault: returned "); outn((u32)r); out(", fs:0 restored "); out(rd_fs0() == 0x11223344u ? "yes" : "NO");
	out(", callee-saved kept "); out((b0 == b1 && s0 == s1 && d0 == d1) ? "yes" : "NO");
	out(", control word "); out(cw1 == cw0 ? "restored" : "NOT restored"); out(", x87 stack top "); outn((u32)((sw >> 11) & 7)); out(" (0 = empty)\n");
	er[1] = 2;
	out("unwinding: handler says "); outn((u32)gc_handler_export(er, (void *)tib, ctx, 0)); out(" (1 = continue search)\n");
	sys3(1, 0, 0, 0);
}
