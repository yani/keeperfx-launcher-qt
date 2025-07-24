
## Translating the KeeperFX Launcher

KeeperFX has players all over the world.
There are quite some players that do not understand English and
we would like to be able to help them enjoy the game.
For this we translate the game and the launcher into the most common languages.

The KeeperFX launcher uses the GNU Gettext format (PO and POT files) to be consistent with KeeperFX.
POT files are the translation templates from which you can make a translation file for a specific language (PO file).

The best way of translating is to use a tool such as [POEdit](https://poedit.net/).
There are many other tools but this is the one most of our translators use.
You can find instructions on how to use POEdit below.

If you need help with specific translations, using both [DeepL](https://www.deepl.com) and [ChatGPT](https://chatgpt.com/)
together with your own understanding of the language seems to be an easy way to get the correct translations. 
You could also contact the maintainer of the language to get some input on your translations.

If any translations are not clear or you don't know how they are used,
feel free to ask the maintainer of the language or ask us in the [Keeper Klan Discord](https://discord.gg/hE4p7vy2Hb).
We're always ready to help you out.

Feel free to submit PRs for translations or
contact Yani on the Keeper Klan Discord to submit any translations you might have.



### Languages and Maintainers

The current languages that we are translating for the Launcher are
the ones that are also present in KeeperFX.
Other languages are allowed too, but if they are not in KeeperFX we might
hold out on adding them until the game also supports the language.

Maintainers are the people in charge of a language translation.
They will not always be the ones that do the translations, but they will be contacted
when we need a proof-read or a quick translation.

| Language              | Code     | Completed | Done/Total     | Maintainer(s)                                    |
|-----------------------|----------|-----------|----------------|--------------------------------------------------|
| English               | EN       | 100%      | Base           | [Yani](https://github.com/yani)                  |
| Dutch                 | NL       | 100%      | 307/307        | [Yani](https://github.com/yani)                  |
| Italian               | IT       |           |                |                                                  |
| French                | FR       | 98.7%     | 303/307        | AncientWay                                       |
| Spanish               | ES       | 100%      | 307/307        | Alniarez                                         |
| German                | DE       |           |                | Aqua, Dofi                                       |
| Polish                | PL       |           |                |                                                  |
| Swedish               | SV       |           |                |                                                  |
| Japanese              | JA       |           |                | [AdamPlenty](https://github.com/AdamPlenty)      |
| Russian               | RU       |           |                |                                                  |
| Korean                | KO       | 100%      | 307/307        | Desert                                           |
| Chinese (Simplified)  | ZH-HANS  | 99.3%     | 305/307        | [JieLeTian](https://github.com/jieletian)        |
| Chinese (Traditional) | ZH-HANT  |           |                |                                                  |
| Czech                 | CS       | 100%      | 307/307        | Gotrek                                           |
| Latin                 | LA       |           |                |                                                  |
| Ukrainian             | UK       | 99.3%     | 305/307        | Mr.Negative, Renegade_Glitch                     |



### Translating with POEdit

First download the [i18n/translations.pot](../i18n/translations.pot) file which is the template for the translations.

Then open up POEdit and start a new translation by selecting `File > New From POT/PO File...`
and selecting the `translations.pot` file.
You can then start translating all of the strings to the language you want to help us with.

Be sure to check out the keybinds under the `Go` menu item. You can mark the current translation as Done
and then move to the next one that needs work by pressing `Ctrl+Return`.
This is a good one to know.

When new translation strings are added or when existing ones are updated,
you can select `Translation > Update from POT file...` to update the list. 
POEdit will mark the ones that need work.



### Testing your translation

It's possible to load a language file directly into the launcher with the following command line option:
```
keeperfx-launcher-qt.exe --language-file=my_translation.po
```



### Thanks

Translations are a community effort and we're very thankful for anybody who can help us out.
