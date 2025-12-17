\page docs-standards Documentation Style Standards

This page lists the documentation standards that you must follow when making PRs for files in `docpages`, this does not mean documentation for functions/variables inside actual code. If something is not covered here, ask on the [official Discord server](https://discord.gg/dpp)!

Please also follow the \ref coding-standards for your PRs related to docs. This covers any code stuff that you may end up writing along with other stuff (for example, conventional commits).

## Adding Pages

All pages should start off with `\ page <page_name> <page title>`, with the space between `\` and `page` removed.

Adding a new page requires you to also add the page as a subpage to a section. For example, if you're creating a new page under "The Basics", you should edit "the_basics.md" inside "example_programs" and add your page as a subpage.

All subpages should be added like so: `* \ subpage <page_name>`, with the space between `\` and `subpage` removed.

## Page and File Names

The naming convention for page and file names are `kebab-case`, so you should name pages and files something like `name-of-page`, with `.md` added to the end of the file name.

However, code files should be `snake_case` to follow the \ref coding-standards. Images follow the same format.

\note There are pages that do not follow this format, **please do not change them**. This is explained in the **Renaming Files** section.

## Page Titles

Page titles shouldn't be mistaken with page names! Page titles are the, sometimes, long-ish titles you see in the navbar on the left of the docs page or at the top of each page when you view it in a web browser. Try not to make these too long (80 chars is the max limit but reach for less) as it's a bit daunting seeing a long title!

## Renaming Files

Renaming `.md` files is something that shouldn't really be done unless you have a very good reason. Changing it harms our SEO, meaning google will get a bit confused.

Changing images in the `images` folder or files inside the `example_code` folder is more acceptable, however, still shouldn't be done without reason.

## Referencing Other Pages

Pages located within our file structure should be referenced like `\ ref <page_name>` (this will insert a reference with the actual page title) or `\ ref <page_name> "text here"` (instead of the page title, it'll be whatever is in quotation marks), with the space between `\` and `ref` removed. Pages outside of our file structure (like a discord invite link, or a github page, etc) should be referenced like `[text](url)`.

## Adding Code to Examples

All code needs to be a `.cpp` file in the `example_code` folder. You then reference it via `\ include{cpp} file.cpp`, with the space between `\` and `include{cpp}` removed.

Any code that will **not** build, for example:
```cpp
bot.start(dpp::st_wait);
/* This code will not build as it has no entry (int main), which will cause CI fails. */
```
should be placed in the file itself. This is so we do not have to worry about the CI testing your example when we know it will not work.

Your examples **need** to be tested. For more information about this, visit the **Testing Your Changes** section.

## Language (Not Programming)

Your text and images should be in English.

## Images

Images are a great way to show people how their bot should act when they're finished with the tutorial! A lot of tutorials don't have them, however, we recommend that you add some as, not only does it tell us that you've tested your example, but it helps users know if they've went wrong! We ask that you don't add images that are inappropriate and only add them if they make sense. Cut them down to a size that is readable and make sure it only contains information that is needed, for example, we don't need to see your server's channels list if the tutorial isn't covering it!

Take a look at some tutorial pages (for example, \ref embed-message and \ref callback-functions) to understand what we mean.

All your images should be placed in the `images` folder and should be named similar to your page's name. If you have multiple images, you can either do something like `image_name2`, `image_name3`, and so on. You can also give them unique names like `image_name_overview`, `image_name_end`, and so on.

## Grammar and Spelling

Your spelling and grammar matters a lot, it's essential that you get it right. We do have a GitHub Action runner that may flag for incorrect spellings if you accidentally spell something incorrectly (We're all human!), so keep an eye out for that. If this falsely flags you, read the \ref coding-standards page for more information about how to tell it to ignore a word.

## Consistency

Try to match other pages if you are making a similar guide (for example, the building from source pages). If the pages are inconsistent, they will start to confuse people. This also goes for pages that may not even be similar! For example, you should try keep your code examples similar to others, so that people don't get confused and can easily follow along.

## Being Simplistic

As you'll be writing tutorials, try be simplistic! Don't overcomplicate pages and don't make assumptions that, just because you know it, everyone else will, because some people may not know what you're talking about! An easy, yet informational, tutorial is key to ensuring that D++ has great documentation for newcomers.

## AI and Desktop Utilities

We are strongly against the use of AI within any part of code and docs. AI can be unreliable and may give the wrong information, so please look to research and test what you're documenting, without the use of AI. If we feel your entire PR was wrote by AI, we will close the PR.

We also advise you do not use grammarly or other tools that fix your grammar/spelling for you. They can mess with text, get it wrong, not understand the context, etc. Whilst you can certainly use spell checkers and/or tools to see if there's issues with your text, you should look over each issue yourself to make sure if it matches the context and the issue is actually correct.

## Testing Your Changes

To test your commits, you should install `doxygen` and `graphviz`. Then, inside your DPP folder, run `doxygen`. This will start generating a bunch of files inside a `docs` folder and may take a while. After that, view the .md file in your web browser.

If this seems to fail, you can make your PR draft and use the netlify bot that replies with a link (the link may take a bit to generate) to preview your changes.

If you are adding code with your commits, you **need** to test that it actually works with a bot. Our CI will test compile it but will not run it, we expect that you test it on your system and ensure you are giving the correct information to users.
