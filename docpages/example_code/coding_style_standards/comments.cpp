/*
 * @brief This is a function that does some cool stuff.
 * More stuff here will still go in brief!
 * @warning This does nothing!
 */
func_name();

/*
 * @brief This turns a name into a meme name!
 *
 * @param name The name of the user that you want to meme-ify.
 * @return a meme name!
 */
std::string name_to_meme(const std::string& name) const;

/* -------------------- .cpp file -------------------- */

int main() {
	/* We are now going to do some cool stuff. */
	func_name();

	/* Going to turn brain into a meme name.
	 * Why?
	 * Because why not. That's why.
	 */
	std::cout << name_to_meme("Brain") << "\n";
}
