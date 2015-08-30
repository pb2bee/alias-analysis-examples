int main() {
	int a[5] = {2, 3, 5, 7, 11};
	int b[5] = {13, 17, 19, 23, 29};
	int sum_a;
	int sum_b;
	int ia, ib;
	for (ia = 0; ia < 5; ia++) {
		sum_a += a[ia];
	}
	for (ib = 0; ib < 5; ib++) {
		sum_b += b[ib];
	}
	return sum_a + sum_b;
}
