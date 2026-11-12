/* sb_exact.c - the EXACT crouch-charged landing model (MCC halo2.dll 1.3528 / Havok, h2sb_core.h; HCM H2CV_SbScan.cpp),
 * in plain C for the Xbox Spooky Ghost build: double math only (x87-native), no libc / libm, no SSE.
 *
 * The model (verified on 1,000+ recorded landings - references/model.md):
 *  - the landing tick collides BEFORE integrating: the pill at its start-of-tick origin as a convex sweep shape = the
 *    capsule (axis feet+0.175 .. feet+0.55, r 0.175) extended by sweep = pre-solve velocity * dt (only when |v| > 5.4465)
 *  - contacts against the flat surface polygons: least overlap (SAT) when the cores overlap -> depth -(overlap + r + 0.01);
 *    else closest points -> depth (distance - r - 0.01), kept when < 0.0333; one contact per surface
 *  - Havok's order on the landing tick: deepest first, each point coupled with the one before it unless that one is
 *  - solver: rhs = min(0.05|d|, dt) * 8/dt; 8 passes, tau 0.6, damp 1, coupled 2x2 with 0.99 n0.n1, pushes only,
 *    rhs -= n.v after each pass; the body leaves with the AVERAGE of the 8 pass velocities
 *  - all contacts on two-sided / invisible surfaces: you fall through (0 of 65 logged launches were soft-only)
 *
 * Host builds (-DHOST) use libm's sqrt; the Xbox build gets the x87 fsqrt from spooky.c (SBX_SQRT). */

#ifndef SBX_TYPES
#define SBX_TYPES
typedef int s32;
typedef unsigned int u32;
#endif
#ifndef SBX_SQRT
#ifdef HOST
#include <math.h>
#define SBX_SQRT(x) sqrt(x)
#else
static double sbx_sqrt_x87(double x) { double r; __asm__("fsqrt" : "=t"(r) : "0"(x)); return r; }
#define SBX_SQRT(x) sbx_sqrt_x87(x)
#endif
#endif

#define SBX_G 4.1712
#define SBX_SWEEP_MIN 5.4465
#define SBX_R_TRI 0.01
#define SBX_TOL 0.0333
#define SBX_TAU 0.6
#define SBX_DAMP 1.0
#define SBX_COUPLE 0.99
#define SBX_PASSES 8
#define SBX_CAP_LO 0.175
#define SBX_CAP_HI 0.55
#define SBX_CAP_R 0.175
#define SBX_MAX_POLY 16
#define SBX_MAX_ROWS 32

typedef struct
{
	double n[3];                 /* the surface's plane normal (unit) */
	double v[SBX_MAX_POLY][3];   /* its vertices, in order */
	s32 nv;
	double lo[3], hi[3];         /* bounds */
	s32 soft;                    /* two-sided / invisible: can't launch on its own */
	u32 key;                     /* the caller's id (surface index) */
} sbx_surf;

typedef struct { double n[3], d; s32 pair, soft; u32 key; } sbx_row;

static double sbx_dot(const double *a, const double *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void sbx_cross(const double *a, const double *b, double *o) { o[0] = a[1] * b[2] - a[2] * b[1]; o[1] = a[2] * b[0] - a[0] * b[2]; o[2] = a[0] * b[1] - a[1] * b[0]; }
static void sbx_sub(const double *a, const double *b, double *o) { o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2]; }
static void sbx_madd(const double *a, const double *b, double s, double *o) { o[0] = a[0] + b[0] * s; o[1] = a[1] + b[1] * s; o[2] = a[2] + b[2] * s; }
static double sbx_len(const double *a) { return SBX_SQRT(sbx_dot(a, a)); }
static double sbx_fabs(double x) { return x < 0 ? -x : x; }
static double sbx_clamp01(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
static s32 sbx_norm(const double *a, double *o)
{
	double l = sbx_len(a);
	if (l < 1e-12) return 0;
	o[0] = a[0] / l; o[1] = a[1] / l; o[2] = a[2] / l;
	return 1;
}

/* fill a surface's bounds from its vertices */
static void sbx_surf_bounds(sbx_surf *s)
{
	s32 i, k;
	for (k = 0; k < 3; k++) s->lo[k] = s->hi[k] = s->v[0][k];
	for (i = 1; i < s->nv; i++) for (k = 0; k < 3; k++) { if (s->v[i][k] < s->lo[k]) s->lo[k] = s->v[i][k]; if (s->v[i][k] > s->hi[k]) s->hi[k] = s->v[i][k]; }
}

static s32 sbx_point_in_poly(const double *p, const sbx_surf *s, const double *n)
{
	s32 i;
	for (i = 0; i < s->nv; i++)
	{
		double e[3], w[3], c[3];
		sbx_sub(s->v[(i + 1) % s->nv], s->v[i], e); sbx_sub(p, s->v[i], w); sbx_cross(e, w, c);
		if (sbx_dot(c, n) < -1e-9) return 0;
	}
	return 1;
}

static void sbx_seg_seg(const double *p1, const double *q1, const double *p2, const double *q2, double *c1, double *c2)
{
	double d1[3], d2[3], r[3], a, e, f, s, t;
	sbx_sub(q1, p1, d1); sbx_sub(q2, p2, d2); sbx_sub(p1, p2, r);
	a = sbx_dot(d1, d1); e = sbx_dot(d2, d2); f = sbx_dot(d2, r);
	if (a < 1e-12 && e < 1e-12) { c1[0] = p1[0]; c1[1] = p1[1]; c1[2] = p1[2]; c2[0] = p2[0]; c2[1] = p2[1]; c2[2] = p2[2]; return; }
	if (a < 1e-12) { s = 0; t = sbx_clamp01(f / e); }
	else
	{
		double cc = sbx_dot(d1, r);
		if (e < 1e-12) { t = 0; s = sbx_clamp01(-cc / a); }
		else
		{
			double b = sbx_dot(d1, d2), den = a * e - b * b;
			s = den > 1e-12 ? sbx_clamp01((b * f - cc * e) / den) : 0.0;
			t = (b * s + f) / e;
			if (t < 0) { t = 0; s = sbx_clamp01(-cc / a); }
			else if (t > 1) { t = 1; s = sbx_clamp01((b - cc) / a); }
		}
	}
	sbx_madd(p1, d1, s, c1); sbx_madd(p2, d2, t, c2);
}

/* one surface against the (swept) pill core: the contact normal and depth, or 0 (none within tolerance) */
static s32 sbx_contact(const double core[4][3], s32 nc, const sbx_surf *s, double *n_out, double *d_out)
{
	double axes[64][3], cd[2][3], best = 1e30, bn[3] = { 0, 0, 0 }, bd = 1e30, ba[3] = { 0, 0, 0 }, bb[3] = { 0, 0, 0 }, pd, dist;
	s32 na = 0, ncd = 0, i, k, sg, j, nce;
	static const s32 ce[4][2] = { {0, 1}, {2, 3}, {0, 2}, {1, 3} };
	axes[na][0] = s->n[0]; axes[na][1] = s->n[1]; axes[na][2] = s->n[2]; na++;
	sbx_sub(core[1], core[0], cd[ncd]); ncd++;
	if (nc == 4) { double pn[3], c[3]; sbx_sub(core[2], core[0], cd[ncd]); ncd++; sbx_cross(cd[0], cd[1], c); if (sbx_norm(c, pn)) { axes[na][0] = pn[0]; axes[na][1] = pn[1]; axes[na][2] = pn[2]; na++; } }
	for (i = 0; i < ncd; i++)
		for (k = 0; k < s->nv; k++)
		{
			double e[3], c[3], ax[3];
			sbx_sub(s->v[(k + 1) % s->nv], s->v[k], e); sbx_cross(cd[i], e, c);
			if (na < 64 && sbx_norm(c, ax)) { axes[na][0] = ax[0]; axes[na][1] = ax[1]; axes[na][2] = ax[2]; na++; }
		}
	for (i = 0; i < na; i++)
		for (sg = -1; sg <= 1; sg += 2)
		{
			double n[3] = { axes[i][0] * sg, axes[i][1] * sg, axes[i][2] * sg }, mp = -1e30, mc = 1e30;
			for (k = 0; k < s->nv; k++) { double y = sbx_dot(n, s->v[k]); if (y > mp) mp = y; }
			for (k = 0; k < nc; k++) { double y = sbx_dot(n, core[k]); if (y < mc) mc = y; }
			if (mp - mc < best) { best = mp - mc; bn[0] = n[0]; bn[1] = n[1]; bn[2] = n[2]; }
		}
	if (best > 0) { n_out[0] = bn[0]; n_out[1] = bn[1]; n_out[2] = bn[2]; *d_out = -(best + SBX_CAP_R + SBX_R_TRI); return 1; }
	/* no overlap: the closest points */
#define SBX_CONSIDER(pa, pb) do { double dd_[3], dd; sbx_sub(pa, pb, dd_); dd = sbx_dot(dd_, dd_); if (dd < bd) { bd = dd; ba[0] = (pa)[0]; ba[1] = (pa)[1]; ba[2] = (pa)[2]; bb[0] = (pb)[0]; bb[1] = (pb)[1]; bb[2] = (pb)[2]; } } while (0)
	pd = sbx_dot(s->n, s->v[0]);
	for (k = 0; k < nc; k++) { double q[3]; sbx_madd(core[k], s->n, -(sbx_dot(s->n, core[k]) - pd), q); if (sbx_point_in_poly(q, s, s->n)) SBX_CONSIDER(core[k], q); }
	nce = nc == 4 ? 4 : 1;
	for (i = 0; i < s->nv; i++)
		for (j = 0; j < nce; j++) { double a[3], b[3]; sbx_seg_seg(core[ce[j][0]], core[ce[j][1]], s->v[i], s->v[(i + 1) % s->nv], a, b); SBX_CONSIDER(a, b); }
	if (nc == 4)
	{
		double e1[3], e2[3], c[3], pn[3];
		sbx_sub(core[1], core[0], e1); sbx_sub(core[2], core[0], e2); sbx_cross(e1, e2, c);
		if (sbx_norm(c, pn))
		{
			sbx_surf quad; double pnn[3] = { -pn[0], -pn[1], -pn[2] };
			quad.nv = 4;
			for (k = 0; k < 3; k++) { quad.v[0][k] = core[0][k]; quad.v[1][k] = core[1][k]; quad.v[2][k] = core[3][k]; quad.v[3][k] = core[2][k]; }
			for (k = 0; k < s->nv; k++)
			{
				double q[3];
				sbx_madd(s->v[k], pn, -(sbx_dot(pn, s->v[k]) - sbx_dot(pn, core[0])), q);
				if (sbx_point_in_poly(q, &quad, pn) || sbx_point_in_poly(q, &quad, pnn)) SBX_CONSIDER(q, s->v[k]);
			}
		}
	}
#undef SBX_CONSIDER
	dist = SBX_SQRT(bd) - SBX_CAP_R - SBX_R_TRI;
	if (dist >= SBX_TOL) return 0;
	{ double d[3]; sbx_sub(ba, bb, d); if (!sbx_norm(d, n_out)) { n_out[0] = s->n[0]; n_out[1] = s->n[1]; n_out[2] = s->n[2]; } }
	*d_out = dist;
	return 1;
}

/* Havok's contact solver on the landing tick: the exit velocity (the average of the 8 passes) */
static void sbx_solve(const sbx_row *rows, s32 n, const double *v0, double dt, double *out)
{
	double v[3] = { v0[0], v0[1], v0[2] }, vsum[3] = { 0, 0, 0 }, rhs[SBX_MAX_ROWS];
	s32 i, pass;
	for (i = 0; i < n; i++) { double m = 0.05 * sbx_fabs(rows[i].d); if (m > dt) m = dt; rhs[i] = rows[i].d < 0 ? m * 8.0 / dt : 0.0; }
	for (pass = 0; pass < SBX_PASSES; pass++)
	{
		for (i = 0; i < n;)
		{
			if (i + 1 < n && rows[i + 1].pair)
			{
				const double *a = rows[i].n, *b = rows[i + 1].n;
				double cp = SBX_COUPLE * sbx_dot(a, b), d0 = SBX_TAU * rhs[i] - SBX_DAMP * sbx_dot(a, v), d1 = SBX_TAU * rhs[i + 1] - SBX_DAMP * sbx_dot(b, v);
				double det = 1.0 - cp * cp, l0 = (d0 - cp * d1) / det;
				if (l0 <= 0) { if (d1 > 0) sbx_madd(v, b, d1, v); }
				else
				{
					double l1 = (d1 - cp * d0) / det;
					if (l1 > 0) { sbx_madd(v, a, l0, v); sbx_madd(v, b, l1, v); }
					else if (d0 > 0) sbx_madd(v, a, d0, v);
				}
				i += 2;
			}
			else
			{
				double d = SBX_TAU * rhs[i] - SBX_DAMP * sbx_dot(rows[i].n, v);
				if (d > 0) sbx_madd(v, rows[i].n, d, v);
				i++;
			}
		}
		for (i = 0; i < n; i++) rhs[i] -= sbx_dot(rows[i].n, v);
		vsum[0] += v[0]; vsum[1] += v[1]; vsum[2] += v[2];
	}
	out[0] = vsum[0] / SBX_PASSES; out[1] = vsum[1] / SBX_PASSES; out[2] = vsum[2] / SBX_PASSES;
}

/* the landing tick's contacts for a pill whose feet start the tick at `feet` with pre-solve velocity `vpre`, against the
 * surfaces S[0..ns): sorted deepest first and paired Havok's way. Returns the count (at most maxr). */
static s32 sbx_landing_contacts(const double *feet, const double *vpre, double dt, const sbx_surf *S, s32 ns, sbx_row *rows, s32 maxr)
{
	double core[4][3], lo[3], hi[3], mg = SBX_CAP_R + 0.05;
	s32 nc = 2, n = 0, i, k, prev_paired;
	core[0][0] = feet[0]; core[0][1] = feet[1]; core[0][2] = feet[2] + SBX_CAP_LO;
	core[1][0] = feet[0]; core[1][1] = feet[1]; core[1][2] = feet[2] + SBX_CAP_HI;
	if (sbx_len(vpre) > SBX_SWEEP_MIN) { sbx_madd(core[0], vpre, dt, core[2]); sbx_madd(core[1], vpre, dt, core[3]); nc = 4; }
	for (k = 0; k < 3; k++) lo[k] = hi[k] = core[0][k];
	for (i = 1; i < nc; i++) for (k = 0; k < 3; k++) { if (core[i][k] < lo[k]) lo[k] = core[i][k]; if (core[i][k] > hi[k]) hi[k] = core[i][k]; }
	for (i = 0; i < ns; i++)
	{
		const sbx_surf *s = &S[i];
		double nn[3], d;
		if (s->hi[0] < lo[0] - mg || s->lo[0] > hi[0] + mg || s->hi[1] < lo[1] - mg || s->lo[1] > hi[1] + mg || s->hi[2] < lo[2] - mg || s->lo[2] > hi[2] + mg) continue;
		if (!sbx_contact(core, nc, s, nn, &d)) continue;
		if (n >= maxr) break;
		rows[n].n[0] = nn[0]; rows[n].n[1] = nn[1]; rows[n].n[2] = nn[2]; rows[n].d = d; rows[n].pair = 0; rows[n].soft = s->soft; rows[n].key = s->key; n++;
	}
	/* stable sort, deepest first (insertion: n is small) */
	for (i = 1; i < n; i++)
	{
		sbx_row t = rows[i]; s32 j = i - 1;
		while (j >= 0 && rows[j].d > t.d) { rows[j + 1] = rows[j]; j--; }
		rows[j + 1] = t;
	}
	prev_paired = 1;
	for (k = 0; k < n; k++) { rows[k].pair = k > 0 && !prev_paired; prev_paired = rows[k].pair; }
	return n;
}

/* reachable: on the tick before, the pill's swept path down to this start was clear (no penetrating contact) */
static s32 sbx_reachable(const double *feet, const double *vpre, double dt, const sbx_surf *S, s32 ns)
{
	sbx_row prev[SBX_MAX_ROWS];
	double f0[3];
	s32 n, k;
	sbx_madd(feet, vpre, -dt, f0);
	n = sbx_landing_contacts(f0, vpre, dt, S, ns, prev, SBX_MAX_ROWS);
	for (k = 0; k < n; k++) if (prev[k].d < -0.001) return 0;
	return 1;
}

/* one landing, start to finish: the exit velocity, and what kind of landing it is.
 * Returns: 0 no contact (falls on), 1 soft only (falls through), 2 a landing (out = the exit velocity). */
#define SBX_MONSTER 20.0
#define SBX_DREADNOUGHT 35.0
static s32 sbx_land(const double *feet, const double *vpre, double dt, const sbx_surf *S, s32 ns, sbx_row *rows, s32 *nrows, double *out)
{
	s32 n = sbx_landing_contacts(feet, vpre, dt, S, ns, rows, SBX_MAX_ROWS), k, hard = 0;
	*nrows = n;
	out[0] = vpre[0]; out[1] = vpre[1]; out[2] = vpre[2];
	if (n == 0) return 0;
	for (k = 0; k < n; k++) if (!rows[k].soft) hard = 1;
	if (!hard) return 1;
	sbx_solve(rows, n, vpre, dt, out);
	return 2;
}
