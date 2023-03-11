
typedef int zip_dig[5];
zip_dig cmu = { 1, 5, 2, 1, 3 };
zip_dig mit = { 0, 2, 1, 3, 9 };
zip_dig ucb = { 9, 4, 7, 2, 0 };

int *univ[3] = {mit, cmu, ucb};
int get_univ_digit(int index, int dight){
  return univ[index][dight];
}
