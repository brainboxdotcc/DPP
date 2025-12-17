class DPP_EXPORT my_new_class {
public:
	int hats;
	int clowns;

	my_new_class& set_hats(int new_hats);
	my_new_class& set_clowns(int new_clowns);
};

my_new_class& my_new_class::set_hats(int new_hats) {
	hats = new_hats;
	return *this;
}

my_new_class& my_new_class::set_clowns(int new_clowns) {
	clowns = new_clowns;
	return *this;
}
