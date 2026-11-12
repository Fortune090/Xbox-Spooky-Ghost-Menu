#include <stdio.h>
typedef int s32;
s32 host_rate(float feet_z, float pt_z, float vh, float vz, s32 air, float jump, s32 walk_ok, s32 jump_ok);
int main(void)
{
	float drops[] = { 0.35f, 0.5f, 0.8f, 1.f, 1.3f, 1.7f, 2.f, 2.5f, 3.f, 4.f, 5.f, 6.5f, 8.f, 10.f, 13.f, 17.f, 22.f, 29.f, 40.f };
	float vhs[] = { 0.f, 2.1f, 2.3f };
	float vzs[] = { 3.f, 0.f, -2.f, -5.f, -9.f, -14.f, -20.f };
	float jumps[] = { 3.08f, 3.3f };
	unsigned i, j, k;
	for (i = 0; i < sizeof drops / sizeof *drops; i++)
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 2; k++) printf("g %.2f %.2f %.2f %d\n", drops[i], vhs[j], jumps[k], host_rate(10.f, 10.f - drops[i], vhs[j], 0.f, 0, jumps[k], 1, 1));
			for (k = 0; k < 7; k++) printf("a %.2f %.2f %.2f %d\n", drops[i], vhs[j], vzs[k], host_rate(10.f, 10.f - drops[i], vhs[j], vzs[k], 1, 3.08f, 1, 1));
		}
	return 0;
}
