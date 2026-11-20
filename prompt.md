# This session has two goals:
1. I want to build a Reaper DAW extension
2. I want to really learn how agentic coding works via the llm cli

# The Reaper Extension goals
The Reaper extension's purpose is to provide git level version control to music. It will do that via the following features:
1. menu item to fetch
2. custom merge conflict algorithm to handle .rpp files
3. have the merge algorithm / diff view show changes via creating new tracks in the reaper file rather than traditional text-based diff view
4. conduct the actual merge after the conflict items are removed
5. menu item to commit after save
6. menu item and panel to push changes to configurable remote

# llm cli goals
I have only used LLMs' chat feature. I want to learn how to optimize my workflow between LLM cli and my editor (vim + lsp + fugitive)
I want to learn keyboard shortcuts and commands to interact with the cli better

# Stack
As a REAPER extension, I want to use C++, git, and git-lfs.
A sample extension is provided in ./sws-project.
The suggested repo to clone for this development is in ./wdl/WDL

# Suggested Workflow
1. I want to suggest a small change
2. view the code
3. read the docs or reference the api provided by wdl that the code uses
4. approve and save the changes
5. test the change in reaper by reloading the extension
6. commit the changes

# First Steps
First, we need to figure out the development process between building a baseline extension and viewing the effects in REAPER
Also, please read and comprehend both the wdl files and the SWS-project to understand how an extension is built
