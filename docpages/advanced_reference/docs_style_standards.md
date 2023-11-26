\page docs_standards Documentation Style Standards

This page lists the documentation standards that you must follow when making PRs for files in `docpages`, this does not mean documentation for functions/variables inside actual code. If something is not covered here, ask on the [official Discord server](https://discord.gg/dpp)!

Please also follow the \ref coding-standards for your PRs related to docs. This covers any code stuff that you may end up writing along with other stuff (for example, conventional commits).

## Adding Pages

All pages should start off with `\ page <page_name> <page title>`, with the space between `\` and `page` removed.

Adding a new page requires you to also add the page as a subpage to a section. For example, if you're creating a new page under "The Basics", you should edit "the_basics.md" inside "example_programs" and add your page as a subpage.

All subpages should be added like so: `* \ subpage <page_name>`, with the space between `\` and `subpage` removed.

## Page Titles and file Names

The naming convention for this follows the same style convention as our code docs (snake case), so you should title pages and name files something like: `name_of_page`.

\note There are pages that do not follow this format, **please do not change them**. This is explained in the section below.

## Renaming files

Renaming `.md` files is something that shouldn't really be done unless you have a very good reason. Changing it harms our SEO, meaning google will get a bit confused.

Changing images or files inside "example_code" is more acceptable, however, still shouldn't be done without reason.

## Referencing Other Pages

Pages located within our file structure should be referenced like `\ ref <page_name>` (this will insert a reference with the actual page title) or `\ ref <page_name> "text here"` (instead of the page title, it'll be whatever is in quotation marks), with the space between `\` and `ref` removed. Pages outside of our file structure (like a discord invite link, or a github page, etc) should be referenced like `[text](url)`.

## Adding Code to Examples

All code needs to be a `.cpp` file in the `example_code` folder. You then reference it via `\ include{cpp} file.cpp`, with the space between `\` and `include{cpp}` removed.

## Language (Not Programming)

Your text and images should be in English.

## Grammar and Spelling

Your spelling and grammar matters a lot, it's essential that you get it right. We do have a GitHub Action runner that may flag for incorrect spellings for if you accidentally spell something incorrectly (We're all human!). If this falsely flags you, read the \ref coding-standards page for more information about how to tell it to ignore a word.

## Consistency

Try to match other pages if you are making a similar guide (for example, the building from source pages). If the pages are inconsistent, they will start to confuse people.

## Being Simplistic

As you'll be writing tutorials, try be simplistic! Don't overcomplicate pages and don't make assumptions that, just because you know it, everyone else will, because some people may not know what you're talking about! An easy, yet informational, tutorial is key to ensuring that D++ has great documentation for newcomers.

## AI and Desktop Utilities

We are strongly against the use of AI within any part of code and docs. AI can be unreliable and may give the wrong information, so please look to research and test what you're documenting, without the use of AI. If we feel your entire PR was wrote by AI, we will close the PR.

We also advise you do not use grammarly or other tools that fix your grammar/spelling for you. They can mess with text, get it wrong, not understand the context, etc. Whilst you can certainly use spell checkers and/or tools to see if there's issues with your text, you should look over each issue yourself to make sure if it matches the context and the issue is actually correct.

## Testing

To test your commits, you should install `doxygen` and `graphviz`. Then, inside your DPP folder, run `doxygen`. This will start generating a bunch of files inside a `docs` folder and may take a while. After that, view the .md file in your web browser.

If this seems to fail, you can make your PR draft and use the netlify bot that replies with a link (the link may take a bit to generate) to preview your changes.

If you are adding code with your commits, you **need** to test it. Our CI will test it too, but we expect that you at least test it on your end.
