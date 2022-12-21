#ifndef __TRIVIALDB_ALGO_SEARCH__
#define __TRIVIALDB_ALGO_SEARCH__

/* find the least x for which p(x) is false,
 * if not found, return hi */
template<typename Predicator>
int lower_bound(int lo, int hi, Predicator p)
{
	if(lo == hi) return hi;

	int rmost = hi--;
	while(lo < hi)
	{
		int mid = lo + (hi - lo) / 2;
		if(!p(mid)) hi = mid;
		else lo = mid + 1;
	}

	return !p(lo) ? lo : rmost;
}

/* find the greatest x for which p(x) is true,
 * if not found, return lo - 1 */
template<typename Predicator>
int upper_bound(int lo, int hi, Predicator p)
{
	if(lo == hi) return lo - 1;

	--hi;
	int lmost = lo;
	while(lo < hi)
	{
		int mid = lo + (hi - lo + 1) / 2;
		if(!p(mid)) hi = mid - 1;
		else lo = mid;
	}

	return !p(lo) ? lmost - 1 : lo;
}

#endif
