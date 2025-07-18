
#include <asm/barrier.h>
#include <linux/compiler.h>
#include <linux/math64.h>
#include <uapi/linux/time.h>

#include "compiler.h"
#include "datapage.h"

#ifdef ARCH_PROVIDES_TIMER
DEFINE_FALLBACK(gettimeofday, struct timeval *, tv, struct timezone *, tz)
#endif
DEFINE_FALLBACK(clock_gettime, clockid_t, clock, struct timespec *, ts)
DEFINE_FALLBACK(clock_getres, clockid_t, clock, struct timespec *, ts)

#ifdef USE_SYSCALL
#if defined(__LP64__)
# define USE_SYSCALL_MASK (USE_SYSCALL | USE_SYSCALL_64)
#else
# define USE_SYSCALL_MASK (USE_SYSCALL | USE_SYSCALL_32)
#endif
#else
# define USE_SYSCALL_MASK ((uint32_t)-1)
#endif

static notrace u32 vdso_read_begin(const struct vdso_data *vd)
{
	u32 seq;

	do {
		seq = READ_ONCE(vd->tb_seq_count);

		if ((seq & 1) == 0)
			break;

		cpu_relax();
	} while (true);

	smp_rmb();
	return seq;
}

static notrace int vdso_read_retry(const struct vdso_data *vd, u32 start)
{
	u32 seq;

	smp_rmb(); 
	seq = READ_ONCE(vd->tb_seq_count);
	return seq != start;
}

static notrace int do_realtime_coarse(const struct vdso_data *vd,
				      struct timespec *ts)
{
	u32 seq;

	do {
		seq = vdso_read_begin(vd);

		ts->tv_sec = vd->xtime_coarse_sec;
		ts->tv_nsec = vd->xtime_coarse_nsec;

	} while (vdso_read_retry(vd, seq));

	return 0;
}

static notrace int do_monotonic_coarse(const struct vdso_data *vd,
				       struct timespec *ts)
{
	struct timespec tomono;
	u32 seq;
	u64 nsec;

	do {
		seq = vdso_read_begin(vd);

		ts->tv_sec = vd->xtime_coarse_sec;
		ts->tv_nsec = vd->xtime_coarse_nsec;

		tomono.tv_sec = vd->wtm_clock_sec;
		tomono.tv_nsec = vd->wtm_clock_nsec;

	} while (vdso_read_retry(vd, seq));

	ts->tv_sec += tomono.tv_sec;

	ts->tv_sec += __iter_div_u64_rem(ts->tv_nsec + tomono.tv_nsec,
					 NSEC_PER_SEC, &nsec);
	ts->tv_nsec = nsec;

	return 0;
}

#ifdef ARCH_PROVIDES_TIMER


static notrace u64 get_clock_shifted_nsec(const u64 cycle_last,
					  const u32 mult,
					  const u64 mask)
{
	u64 res;

	res = arch_vdso_read_counter();

	res = res - cycle_last;

	res &= mask;
	return res * mult;
}

static notrace int do_realtime(const struct vdso_data *vd, struct timespec *ts)
{
	u32 seq, mult, shift;
	u64 nsec, cycle_last;
#ifdef ARCH_CLOCK_FIXED_MASK
	static const u64 mask = ARCH_CLOCK_FIXED_MASK;
#else
	u64 mask;
#endif
	vdso_xtime_clock_sec_t sec;

	do {
		seq = vdso_read_begin(vd);

		if (vd->use_syscall & USE_SYSCALL_MASK)
			return -1;

		cycle_last = vd->cs_cycle_last;

		mult = vd->cs_mono_mult;
		shift = vd->cs_shift;
#ifndef ARCH_CLOCK_FIXED_MASK
		mask = vd->cs_mask;
#endif

		sec = vd->xtime_clock_sec;
		nsec = vd->xtime_clock_snsec;

	} while (unlikely(vdso_read_retry(vd, seq)));

	nsec += get_clock_shifted_nsec(cycle_last, mult, mask);
	nsec >>= shift;

	ts->tv_sec = sec + __iter_div_u64_rem(nsec, NSEC_PER_SEC, &nsec);
	ts->tv_nsec = nsec;

	return 0;
}

static notrace int do_monotonic(const struct vdso_data *vd, struct timespec *ts)
{
	u32 seq, mult, shift;
	u64 nsec, cycle_last;
#ifdef ARCH_CLOCK_FIXED_MASK
	static const u64 mask = ARCH_CLOCK_FIXED_MASK;
#else
	u64 mask;
#endif
	vdso_wtm_clock_nsec_t wtm_nsec;
	__kernel_time_t sec;

	do {
		seq = vdso_read_begin(vd);

		if (vd->use_syscall & USE_SYSCALL_MASK)
			return -1;

		cycle_last = vd->cs_cycle_last;

		mult = vd->cs_mono_mult;
		shift = vd->cs_shift;
#ifndef ARCH_CLOCK_FIXED_MASK
		mask = vd->cs_mask;
#endif

		sec = vd->xtime_clock_sec;
		nsec = vd->xtime_clock_snsec;

		sec += vd->wtm_clock_sec;
		wtm_nsec = vd->wtm_clock_nsec;

	} while (unlikely(vdso_read_retry(vd, seq)));

	nsec += get_clock_shifted_nsec(cycle_last, mult, mask);
	nsec >>= shift;
	nsec += wtm_nsec;

	ts->tv_sec = sec + __iter_div_u64_rem(nsec, NSEC_PER_SEC, &nsec);
	ts->tv_nsec = nsec;

	return 0;
}

static notrace int do_monotonic_raw(const struct vdso_data *vd,
				    struct timespec *ts)
{
	u32 seq, mult, shift;
	u64 nsec, cycle_last;
#ifdef ARCH_CLOCK_FIXED_MASK
	static const u64 mask = ARCH_CLOCK_FIXED_MASK;
#else
	u64 mask;
#endif
	vdso_raw_time_sec_t sec;

	do {
		seq = vdso_read_begin(vd);

		if (vd->use_syscall & USE_SYSCALL_MASK)
			return -1;

		cycle_last = vd->cs_cycle_last;

		mult = vd->cs_raw_mult;
		shift = vd->cs_shift;
#ifndef ARCH_CLOCK_FIXED_MASK
		mask = vd->cs_mask;
#endif

		sec = vd->raw_time_sec;
		nsec = vd->raw_time_nsec;

	} while (unlikely(vdso_read_retry(vd, seq)));

	nsec += get_clock_shifted_nsec(cycle_last, mult, mask);
	nsec >>= shift;

	ts->tv_sec = sec + __iter_div_u64_rem(nsec, NSEC_PER_SEC, &nsec);
	ts->tv_nsec = nsec;

	return 0;
}

static notrace int do_boottime(const struct vdso_data *vd, struct timespec *ts)
{
	u32 seq, mult, shift;
	u64 nsec, cycle_last;
	vdso_wtm_clock_nsec_t wtm_nsec;
#ifdef ARCH_CLOCK_FIXED_MASK
	static const u64 mask = ARCH_CLOCK_FIXED_MASK;
#else
	u64 mask;
#endif
	__kernel_time_t sec;

	do {
		seq = vdso_read_begin(vd);

		if (vd->use_syscall & USE_SYSCALL_MASK)
			return -1;

		cycle_last = vd->cs_cycle_last;

		mult = vd->cs_mono_mult;
		shift = vd->cs_shift;
#ifndef ARCH_CLOCK_FIXED_MASK
		mask = vd->cs_mask;
#endif

		sec = vd->xtime_clock_sec;
		nsec = vd->xtime_clock_snsec;

		sec += vd->wtm_clock_sec + vd->btm_sec;
		wtm_nsec = vd->wtm_clock_nsec + vd->btm_nsec;

	} while (unlikely(vdso_read_retry(vd, seq)));

	nsec += get_clock_shifted_nsec(cycle_last, mult, mask);
	nsec >>= shift;
	nsec += wtm_nsec;


	ts->tv_sec = sec + __iter_div_u64_rem(nsec, NSEC_PER_SEC, &nsec);
	ts->tv_nsec = nsec;

	return 0;
}

#endif

notrace int __vdso_clock_gettime(clockid_t clock, struct timespec *ts)
{
	const struct vdso_data *vd = __get_datapage();

#ifdef USE_SYSCALL
	if (vd->use_syscall & USE_SYSCALL_MASK) {
		goto fallback;
	}
#endif

	switch (clock) {
	case CLOCK_REALTIME_COARSE:
		do_realtime_coarse(vd, ts);
		break;
	case CLOCK_MONOTONIC_COARSE:
		do_monotonic_coarse(vd, ts);
		break;
#ifdef ARCH_PROVIDES_TIMER
	case CLOCK_REALTIME:
		if (do_realtime(vd, ts))
			goto fallback;
		break;
	case CLOCK_MONOTONIC:
		if (do_monotonic(vd, ts))
			goto fallback;
		break;
	case CLOCK_MONOTONIC_RAW:
		if (do_monotonic_raw(vd, ts))
			goto fallback;
		break;
	case CLOCK_BOOTTIME:
		if (do_boottime(vd, ts))
			goto fallback;
		break;
#endif
	default:
		goto fallback;
	}

	return 0;
fallback:
	return clock_gettime_fallback(clock, ts);
}

#ifdef ARCH_PROVIDES_TIMER
notrace int __vdso_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	const struct vdso_data *vd = __get_datapage();

	if (likely(tv != NULL)) {
		struct timespec ts;

		if (do_realtime(vd, &ts))
			return gettimeofday_fallback(tv, tz);

		tv->tv_sec = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}

	if (unlikely(tz != NULL)) {
		tz->tz_minuteswest = vd->tz_minuteswest;
		tz->tz_dsttime = vd->tz_dsttime;
	}

	return 0;
}
#endif

int __vdso_clock_getres(clockid_t clock, struct timespec *res)
{
	long nsec;

#ifdef USE_SYSCALL
	const struct vdso_data *vd = __get_datapage();

	if (vd->use_syscall & USE_SYSCALL_MASK) {
		return clock_getres_fallback(clock, res);
	}
#endif

	switch (clock) {
	case CLOCK_REALTIME_COARSE:
	case CLOCK_MONOTONIC_COARSE:
		nsec = LOW_RES_NSEC;
		break;
#ifdef ARCH_PROVIDES_TIMER
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
	case CLOCK_MONOTONIC_RAW:
	case CLOCK_BOOTTIME:
		nsec = MONOTONIC_RES_NSEC;
		break;
#endif
	default:
		return clock_getres_fallback(clock, res);
	}

	if (likely(res != NULL)) {
		res->tv_sec = 0;
		res->tv_nsec = nsec;
	}

	return 0;
}

notrace time_t __vdso_time(time_t *t)
{
	const struct vdso_data *vd = __get_datapage();

#ifdef USE_SYSCALL
	time_t result;

	if (vd->use_syscall & USE_SYSCALL_MASK) {

		struct timeval tv;
		int ret = gettimeofday_fallback(&tv, NULL);

		if (ret < 0)
			return ret;
		result = tv.tv_sec;
	} else {
		result = READ_ONCE(vd->xtime_coarse_sec);
	}
#else
	time_t result = READ_ONCE(vd->xtime_coarse_sec);
#endif

	if (t)
		*t = result;
	return result;
}
