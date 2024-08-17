import random, itertools, time, ast

# From: https://gist.github.com/shmookey/b28e342e1b1756c4700f42f17102c2ff
words = [
    "ABACK", "ABAFT", "ABASE", "ABATE", "ABBEY", "ABBOT", "ABHOR", "ABIDE", "ABLER", "ABODE", 
    "ABOUT", "ABOVE", "ABUSE", "ABYSS", "ACHED", "ACHES", "ACIDS", "ACORN", "ACRES", "ACRID", 
    "ACTED", "ACTOR", "ACUTE", "ADAGE", "ADAPT", "ADDED", "ADDER", "ADEPT", "ADIEU", "ADMIT", 
    "ADOBE", "ADOPT", "ADORE", "ADORN", "ADULT", "AEGIS", "AEONS", "AFFIX", "AFIRE", "AFOOT", 
    "AFTER", "AGAIN", "AGAPE", "AGATE", "AGENT", "AGILE", "AGING", "AGLOW", "AGONY", "AGREE", 
    "AHEAD", "AIDED", "AIDES", "AILED", "AIMED", "AIRED", "AISLE", "ALARM", "ALBUM", "ALDER", 
    "ALERT", "ALIAS", "ALIBI", "ALIEN", "ALIKE", "ALIVE", "ALLAY", "ALLEY", "ALLOT", "ALLOW", 
    "ALLOY", "ALOES", "ALOFT", "ALONE", "ALONG", "ALOOF", "ALOUD", "ALPHA", "ALTAR", "ALTER", 
    "ALTOS", "AMASS", "AMAZE", "AMBER", "AMBLE", "AMEND", "AMIGO", "AMISS", "AMITY", "AMONG", 
    "AMOUR", "AMPLE", "AMPLY", "AMUSE", "ANGEL", "ANGER", "ANGLE", "ANGRY", "ANGST", "ANIME", 
    "ANKLE", "ANNEX", "ANNOY", "ANNUL", "ANTES", "ANTIC", "ANVIL", "APACE", "APART", "APING", 
    "APPAL", "APPLE", "APPLY", "APRON", "APTLY", "AREAS", "ARENA", "ARGUE", "ARISE", "ARMED", 
    "AROMA", "AROSE", "ARRAY", "ARROW", "ARSON", "ASHEN", "ASHES", "ASIDE", "ASKED", "ASKEW", 
    "ASPEN", "ASSAY", "ASSES", "ASSET", "ASTER", "ASTIR", "ATLAS", "ATOLL", "ATOMS", "ATONE", 
    "ATTAR", "ATTIC", "AUDIO", "AUDIT", "AUGER", "AUGHT", "AUGUR", "AUNTS", "AURAS", "AUTOS", 
    "AVAIL", "AVERS", "AVERT", "AVOID", "AVOWS", "AWAIT", "AWAKE", "AWARD", "AWARE", "AWFUL", 
    "AWOKE", "AXIOM", "AXLES", "AZURE", "BABEL", "BABES", "BACKS", "BACON", "BADGE", "BADLY", 
    "BAGGY", "BAITS", "BAIZE", "BAKED", "BAKER", "BALES", "BALLS", "BALMY", "BANAL", "BANDS",
    "BANDY", "BANGS", "BANJO", "BANKS", "BANNS", "BARBS", "BARDS", "BARED", "BARGE", "BARKS",
    "BARNS", "BARON", "BASAL", "BASED", "BASER", "BASES", "BASIC", "BASIL", "BASIN", "BASIS",
    "BASSO", "BASTE", "BATCH", "BATED", "BATHE", "BATHS", "BATON", "BAYOU", "BEACH", "BEADS",
    "BEADY", "BEAKS", "BEAMS", "BEANS", "BEARD", "BEARS", "BEAST", "BEAUX", "BEECH", "BEETS",
    "BEFIT", "BEGAN", "BEGAT", "BEGET", "BEGIN", "BEGOT", "BEGUN", "BEING", "BELIE", "BELLE",
    "BELLS", "BELLY", "BELOW", "BELTS", "BENCH", "BENDS", "BERGS", "BERRY", "BERTH", "BERYL",
    "BESET", "BESOM", "BEVEL", "BIBLE", "BIDED", "BIDES", "BIGHT", "BIGOT", "BILGE", "BILLS",
    "BILLY", "BINDS", "BIPED", "BIRCH", "BIRDS", "BIRTH", "BISON", "BITCH", "BITES", "BLACK",
    "BLADE", "BLAME", "BLAND", "BLANK", "BLARE", "BLAST", "BLAZE", "BLEAK", "BLEAT", "BLEED",
    "BLEND", "BLENT", "BLESS", "BLEST", "BLIND", "BLINK", "BLISS", "BLOCK", "BLOCS", "BLOND",
    "BLOOD", "BLOOM", "BLOTS", "BLOWN", "BLOWS", "BLUER", "BLUES", "BLUFF", "BLUNT", "BLURT",
    "BLUSH", "BOARD", "BOARS", "BOAST", "BOATS", "BODED", "BODES", "BOGGY", "BOGUS", "BOILS",
    "BOLES", "BOLTS", "BOMBS", "BONDS", "BONED", "BONES", "BONNY", "BONUS", "BOOBY", "BOOKS",
    "BOOMS", "BOONS", "BOORS", "BOOST", "BOOTH", "BOOTS", "BOOTY", "BOOZE", "BORAX", "BORED",
    "BORES", "BORNE", "BOSOM", "BOUGH", "BOUND", "BOUTS", "BOWED", "BOWEL", "BOWER", "BOWLS",
    "BOXED", "BOXER", "BOXES", "BRACE", "BRAGS", "BRAID", "BRAIN", "BRAKE", "BRAND", "BRASS",
    "BRATS", "BRAVE", "BRAVO", "BRAWL", "BRAWN", "BREAD", "BREAK", "BREED", "BRIAR", "BRIBE",
    "BRICK", "BRIDE", "BRIEF", "BRIER", "BRIGS", "BRIMS", "BRINE", "BRING", "BRINK", "BRINY",
    "BRISK", "BROAD", "BROIL", "BROKE", "BROOD", "BROOK", "BROOM", "BROTH", "BROWN", "BROWS",
    "BRUIN", "BRUNT", "BRUSH", "BRUTE", "BUCKS", "BUDGE", "BUGGY", "BUGLE", "BUILD", "BUILT",
    "BULBS", "BULGE", "BULKS", "BULKY", "BULLS", "BULLY", "BUMPS", "BUNCH", "BUNKS", "BUOYS",
    "BURLY", "BURNS", "BURNT", "BURRO", "BURRS", "BURST", "BUSHY", "BUSTS", "BUTTE", "BUTTS",
    "BUXOM", "BUYER", "CABAL", "CABBY", "CABIN", "CABLE", "CACAO", "CACHE", "CADET", "CADRE",
    "CAGED", "CAGES", "CAIRN", "CAKED", "CAKES", "CALLS", "CALMS", "CALYX", "CAMEL", "CAMEO",
    "CAMPS", "CANAL", "CANDY", "CANES", "CANNY", "CANOE", "CANON", "CANTO", "CAPER", "CAPES",
    "CAPON", "CARDS", "CARED", "CARES", "CARGO", "CAROL", "CARRY", "CARTS", "CARVE", "CASED",
    "CASES", "CASKS", "CASTE", "CASTS", "CATCH", "CATER", "CAUSE", "CAVED", "CAVES", "CAVIL",
    "CEASE", "CEDAR", "CEDED", "CELLS", "CENTS", "CHAFE", "CHAFF", "CHAIN", "CHAIR", "CHALK",
    "CHAMP", "CHANT", "CHAOS", "CHAPS", "CHARM", "CHART", "CHARY", "CHASE", "CHASM", "CHATS",
    "CHEAP", "CHEAT", "CHECK", "CHEEK", "CHEER", "CHEFS", "CHESS", "CHEST", "CHICK", "CHIDE",
    "CHIEF", "CHILD", "CHILL", "CHIME", "CHINA", "CHINK", "CHINS", "CHIPS", "CHIRP", "CHOIR",
    "CHOKE", "CHOPS", "CHORD", "CHOSE", "CHUCK", "CHUMP", "CHUMS", "CHUNK", "CHURL", "CHURN",
    "CHUTE", "CIDER", "CIGAR", "CINCH", "CIRCA", "CITED", "CITES", "CIVET", "CIVIC", "CIVIL",
    "CLACK", "CLAIM", "CLAMP", "CLAMS", "CLANG", "CLANK", "CLANS", "CLAPS", "CLASH", "CLASP",
    "CLASS", "CLAWS", "CLEAN", "CLEAR", "CLEFS", "CLEFT", "CLERK", "CLEWS", "CLICK", "CLIFF",
    "CLIMB", "CLIME", "CLING", "CLINK", "CLIPS", "CLOAK", "CLOCK", "CLODS", "CLOGS", "CLOSE",
    "CLOTH", "CLOUD", "CLOUT", "CLOVE", "CLOWN", "CLUBS", "CLUCK", "CLUES", "CLUMP", "CLUNG",
    "COACH", "COALS", "COAST", "COATS", "COBRA", "COCKS", "COCOA", "CODES", "COILS", "COINS",
    "COLDS", "COLIC", "COLON", "COLTS", "COMBS", "COMER", "COMES", "COMET", "COMIC", "COMMA",
    "CONCH", "CONES", "CONIC", "COOED", "COOKS", "COOLS", "COPRA", "COPSE", "CORAL", "CORDS",
    "CORES", "CORKS", "CORNS", "CORPS", "COSTS", "COTES", "COUCH", "COUGH", "COULD", "COUNT",
    "COUPE", "COUPS", "COURT", "COVER", "COVES", "COVET", "COVEY", "COWED", "COWER", "COYLY",
    "COZEN", "CRABS", "CRACK", "CRAFT", "CRAGS", "CRAMP", "CRANE", "CRANK", "CRAPE", "CRASH",
    "CRASS", "CRATE", "CRAVE", "CRAWL", "CRAZE", "CRAZY", "CREAK", "CREAM", "CREDO", "CREED",
    "CREEK", "CREEP", "CREPE", "CREPT", "CRESS", "CREST", "CREWS", "CRIBS", "CRICK", "CRIED",
    "CRIER", "CRIES", "CRIME", "CRIMP", "CRISP", "CROAK", "CROCK", "CRONE", "CRONY", "CROOK",
    "CROPS", "CROSS", "CROUP", "CROWD", "CROWN", "CROWS", "CRUDE", "CRUEL", "CRUMB", "CRUSH",
    "CRUST", "CRYPT", "CUBES", "CUBIC", "CUBIT", "CUFFS", "CULTS", "CURDS", "CURED", "CURES",
    "CURLS", "CURLY", "CURRY", "CURSE", "CURST", "CURVE", "CYCLE", "CYNIC", "DADDY", "DAILY",
    "DAIRY", "DAISY", "DALES", "DALLY", "DAMES", "DAMPS", "DANCE", "DANDY", "DARED", "DARES",
    "DARTS", "DATED", "DATES", "DATUM", "DAUBS", "DAUNT", "DAWNS", "DAZED", "DEALS", "DEALT",
    "DEANS", "DEARS", "DEATH", "DEBAR", "DEBIT", "DEBTS", "DEBUT", "DECAY", "DECKS", "DECOY",
    "DECRY", "DEEDS", "DEEMS", "DEEPS", "DEFER", "DEIGN", "DEITY", "DELAY", "DELLS", "DELTA",
    "DELVE", "DEMON", "DEMUR", "DENSE", "DENTS", "DEPOT", "DEPTH", "DERBY", "DESKS", "DETER",
    "DEUCE", "DEVIL", "DIARY", "DICED", "DICES", "DICTA", "DIETS", "DIGIT", "DIKES", "DIMES",
    "DIMLY", "DINED", "DINER", "DINES", "DINGY", "DIRGE", "DIRTY", "DISCS", "DISKS", "DITCH",
    "DITTO", "DITTY", "DIVAN", "DIVED", "DIVER", "DIVES", "DIZZY", "DOCKS", "DODGE", "DOERS",
    "DOGMA", "DOING", "DOLED", "DOLLS", "DOMED", "DOMES", "DONOR", "DOOMS", "DOORS", "DOSED",
    "DOSES", "DOTED", "DOTES", "DOUBT", "DOUGH", "DOVES", "DOWDY", "DOWNS", "DOWNY", "DOWRY",
    "DOZED", "DOZEN", "DRAFT", "DRAGS", "DRAIN", "DRAKE", "DRAMA", "DRAMS", "DRANK", "DRAPE",
    "DRAWL", "DRAWN", "DRAWS", "DRAYS", "DREAD", "DREAM", "DREGS", "DRESS", "DRIED", "DRIER",
    "DRIES", "DRIFT", "DRILL", "DRILY", "DRINK", "DRIPS", "DRIVE", "DROLL", "DRONE", "DROOP",
    "DROPS", "DROSS", "DROVE", "DROWN", "DRUGS", "DRUMS", "DRUNK", "DRYLY", "DUCAL", "DUCAT",
    "DUCHY", "DUCKS", "DUCTS", "DUELS", "DUETS", "DUKES", "DULLY", "DUMMY", "DUMPS", "DUMPY",
    "DUNCE", "DUNES", "DUNNO", "DUPED", "DUPES", "DUSKY", "DUSTY", "DWARF", "DWELL", "DWELT",
    "DYING", "DYKES", "EAGER", "EAGLE", "EARLS", "EARLY", "EARNS", "EARTH", "EASED", "EASEL",
    "EASES", "EATEN", "EATER", "EAVES", "EBBED", "EBONY", "EDGED", "EDGES", "EDICT", "EDIFY",
    "EERIE", "EGGED", "EIGHT", "EJECT", "ELATE", "ELBOW", "ELDER", "ELECT", "ELEGY", "ELFIN",
    "ELITE", "ELOPE", "ELUDE", "ELVES", "EMAIL", "EMITS", "EMPTY", "ENACT", "ENDED", "ENDOW",
    "ENEMY", "ENJOY", "ENNUI", "ENROL", "ENSUE", "ENTER", "ENTRY", "ENVOY", "EPICS", "EPOCH",
    "EQUAL", "EQUIP", "ERASE", "ERECT", "ERRED", "ERROR", "ESSAY", "ETHER", "ETHIC", "EVADE",
    "EVENT", "EVERY", "EVILS", "EVOKE", "EXACT", "EXALT", "EXCEL", "EXERT", "EXILE", "EXIST",
    "EXITS", "EXPEL", "EXTOL", "EXTRA", "EXULT", "EYING", "EYRIE", "FABLE", "FACED", "FACES",
    "FACTS", "FADED", "FADES", "FAILS", "FAINT", "FAIRS", "FAIRY", "FAITH", "FAKIR", "FALLS",
    "FALSE", "FAMED", "FANCY", "FANGS", "FARCE", "FARED", "FARES", "FARMS", "FASTS", "FATAL",
    "FATED", "FATES", "FATTY", "FAULT", "FAUNA", "FAUNS", "FAWNS", "FEARS", "FEAST", "FEATS",
    "FEEDS", "FEELS", "FEIGN", "FEINT", "FELLS", "FELON", "FENCE", "FERAL", "FERNS", "FERRY",
    "FETCH", "FETED", "FETID", "FETUS", "FEUDS", "FEVER", "FEWER", "FICHE", "FIEFS", "FIELD",
    "FIEND", "FIERY", "FIFES", "FIFTH", "FIFTY", "FIGHT", "FILCH", "FILED", "FILES", "FILET",
    "FILLS", "FILLY", "FILMS", "FILMY", "FILTH", "FINAL", "FINCH", "FINDS", "FINED", "FINER",
    "FINES", "FINIS", "FINNY", "FIORD", "FIRED", "FIRES", "FIRMS", "FIRST", "FISHY", "FISTS",
    "FITLY", "FIVES", "FIXED", "FIXER", "FIXES", "FJORD", "FLAGS", "FLAIL", "FLAIR", "FLAKE",
    "FLAKY", "FLAME", "FLANK", "FLAPS", "FLARE", "FLASH", "FLASK", "FLATS", "FLAWS", "FLEAS",
    "FLECK", "FLEES", "FLEET", "FLESH", "FLICK", "FLIER", "FLIES", "FLING", "FLINT", "FLIRT",
    "FLITS", "FLOAT", "FLOCK", "FLOES", "FLOOD", "FLOOR", "FLORA", "FLOSS", "FLOUR", "FLOUT",
    "FLOWN", "FLOWS", "FLUES", "FLUFF", "FLUID", "FLUKE", "FLUME", "FLUNG", "FLUSH", "FLUTE",
    "FLYER", "FOAMS", "FOAMY", "FOCAL", "FOCUS", "FOGGY", "FOILS", "FOIST", "FOLDS", "FOLIO", 
    "FOLKS", "FOLLY", "FOODS", "FOOLS", "FORAY", "FORCE", "FORDS", "FORGE", "FORGO", "FORKS",
    "FORMS", "FORTE", "FORTH", "FORTS", "FORTY", "FORUM", "FOUND", "FOUNT", "FOURS", "FOWLS",
    "FOXES", "FOYER", "FRAIL", "FRAME", "FRANC", "FRANK", "FRAUD", "FREAK", "FREED", "FREER",
    "FREES", "FRESH", "FRETS", "FRIAR", "FRIED", "FRILL", "FRISK", "FROCK", "FROGS", "FROND",
    "FRONT", "FROST", "FROTH", "FROWN", "FROZE", "FRUIT", "FUDGE", "FUELS", "FUGUE", "FULLY",
    "FUMED", "FUMES", "FUNDS", "FUNGI", "FUNNY", "FURRY", "FURZE", "FUSED", "FUSES", "FUSSY",
    "FUZZY", "GABLE", "GAILY", "GAINS", "GALES", "GALLS", "GAMES", "GAMIN", "GAMMA", "GAMUT",
    "GANGS", "GAPED", "GAPES", "GASES", "GASPS", "GATES", "GAUDY", "GAUGE", "GAUNT", "GAUZE",
    "GAUZY", "GAVEL", "GAWKY", "GAYER", "GAYLY", "GAZED", "GAZER", "GAZES", "GEARS", "GEESE",
    "GENIE", "GENII", "GENRE", "GENTS", "GENUS", "GERMS", "GHOST", "GIANT", "GIBES", "GIDDY",
    "GIFTS", "GILDS", "GILLS", "GIMME", "GIPSY", "GIRDS", "GIRLS", "GIRTH", "GIVEN", "GIVES",
    "GLADE", "GLAND", "GLARE", "GLASS", "GLAZE", "GLEAM", "GLEAN", "GLENS", "GLIDE", "GLINT",
    "GLOAT", "GLOBE", "GLOOM", "GLORY", "GLOSS", "GLOVE", "GLOWS", "GLUED", "GNASH", "GNATS",
    "GNAWS", "GNOME", "GOADS", "GOALS", "GOATS", "GODLY", "GOING", "GOLLY", "GONGS", "GONNA",
    "GOODS", "GOODY", "GOOSE", "GORED", "GORGE", "GORSE", "GOTTA", "GOUGE", "GOURD", "GOUTY",
    "GOWNS", "GRABS", "GRACE", "GRADE", "GRAFT", "GRAIN", "GRAMS", "GRAND", "GRANT", "GRAPE",
    "GRAPH", "GRASP", "GRASS", "GRATE", "GRAVE", "GRAVY", "GRAZE", "GREAT", "GREED", "GREEN",
    "GREET", "GREYS", "GRIEF", "GRILL", "GRIME", "GRIMY", "GRIND", "GRINS", "GRIPE", "GRIPS",
    "GRIST", "GROAN", "GROIN", "GROOM", "GROPE", "GROSS", "GROUP", "GROVE", "GROWL", "GROWN",
    "GROWS", "GRUBS", "GRUEL", "GRUFF", "GRUNT", "GUANO", "GUARD", "GUESS", "GUEST", "GUIDE",
    "GUILD", "GUILE", "GUILT", "GUISE", "GULCH", "GULFS", "GULLS", "GULLY", "GUMMY", "GUSTO",
    "GUSTS", "GUSTY", "GYPSY", "HABIT", "HACKS", "HAILS", "HAIRS", "HAIRY", "HALED", "HALLS",
    "HALTS", "HALVE", "HANDS", "HANDY", "HANGS", "HAPPY", "HARDY", "HAREM", "HARES", "HARMS",
    "HARPS", "HARPY", "HARRY", "HARSH", "HARTS", "HASTE", "HASTY", "HATCH", "HATED", "HATER",
    "HAULS", "HAVEN", "HAVOC", "HAWKS", "HAZEL", "HEADS", "HEADY", "HEALS", "HEAPS", "HEARD",
    "HEARS", "HEART", "HEATH", "HEATS", "HEAVE", "HEAVY", "HEDGE", "HEEDS", "HEELS", "HEIRS",
    "HELIX", "HELLO", "HELMS", "HELPS", "HENCE", "HERBS", "HERDS", "HERON", "HEROS", "HEWED",
    "HIDES", "HILLS", "HILLY", "HILTS", "HINDS", "HINGE", "HINTS", "HIRED", "HIRES", "HITCH",
    "HIVES", "HOARD", "HOARY", "HOBBY", "HOIST", "HOLDS", "HOLES", "HOLLY", "HOMES", "HONEY",
    "HOODS", "HOOFS", "HOOKS", "HOOPS", "HOOTS", "HOPED", "HOPES", "HORDE", "HORNS", "HORNY",
    "HORSE", "HOSTS", "HOTEL", "HOTLY", "HOUND", "HOURS", "HOUSE", "HOVEL", "HOVER", "HOWLS",
    "HULKS", "HULLS", "HUMAN", "HUMID", "HUMPS", "HUMUS", "HUNCH", "HUNTS", "HURLS", "HURRY",
    "HURTS", "HUSKS", "HUSKY", "HUSSY", "HYDRA", "HYENA", "HYMNS", "ICILY", "ICING", "IDEAL",
    "IDEAS", "IDIOM", "IDIOT", "IDLED", "IDLER", "IDOLS", "IDYLL", "IGLOO", "IMAGE", "IMBUE",
    "IMPEL", "IMPLY", "INANE", "INCUR", "INDEX", "INEPT", "INERT", "INFER", "INGOT", "INLET",
    "INNER", "INTER", "INURE", "IRATE", "IRKED", "IRONS", "IRONY", "ISLES", "ISLET", "ISSUE",
    "ITEMS", "IVORY", "JACKS", "JADED", "JAILS", "JAUNT", "JEANS", "JEERS", "JELLY", "JERKS",
    "JERKY", "JESTS", "JETTY", "JEWEL", "JIFFY", "JOINS", "JOINT", "JOKED", "JOKER", "JOKES",
    "JOLLY", "JOUST", "JOYED", "JUDGE", "JUICE", "JUICY", "JUMPS", "JUNKS", "JUNTA", "JUROR",
    "KARMA", "KEELS", "KEEPS", "KETCH", "KEYED", "KHAKI", "KICKS", "KILLS", "KINDA", "KINDS",
    "KINGS", "KIOSK", "KITES", "KNACK", "KNAVE", "KNEAD", "KNEEL", "KNEES", "KNELL", "KNELT",
    "KNIFE", "KNITS", "KNOBS", "KNOCK", "KNOLL", "KNOTS", "KNOWN", "KNOWS", "LABEL", "LACED",
    "LACES", "LACKS", "LADEN", "LADLE", "LAGER", "LAIRS", "LAITY", "LAKES", "LAMBS", "LAMED",
    "LAMES", "LAMPS", "LANCE", "LANDS", "LANES", "LANKY", "LAPEL", "LAPSE", "LARCH", "LARGE",
    "LARGO", "LARKS", "LARVA", "LASSO", "LASTS", "LATCH", "LATER", "LATHE", "LATHS", "LAUGH",
    "LAWNS", "LAYER", "LEADS", "LEAFY", "LEAKS", "LEAKY", "LEANS", "LEAPS", "LEAPT", "LEARN",
    "LEASE", "LEASH", "LEAST", "LEAVE", "LEDGE", "LEECH", "LEEKS", "LEGAL", "LEMME", "LEMON",
    "LENDS", "LEPER", "LEVEE", "LEVEL", "LEVER", "LIARS", "LIBEL", "LICKS", "LIEGE", "LIENS",
    "LIFTS", "LIGHT", "LIKED", "LIKEN", "LIKER", "LIKES", "LILAC", "LIMBO", "LIMBS", "LIMES",
    "LIMIT", "LINED", "LINEN", "LINER", "LINES", "LINGO", "LINKS", "LIONS", "LISTS", "LITHE",
    "LIVED", "LIVER", "LIVES", "LIVID", "LLAMA", "LOADS", "LOAMY", "LOANS", "LOATH", "LOBBY",
    "LOBES", "LOCAL", "LOCKS", "LOCUS", "LODGE", "LOFTY", "LOGES", "LOGIC", "LOGIN", "LOINS",
    "LONGS", "LOOKS", "LOOMS", "LOONS", "LOOPS", "LOOSE", "LORDS", "LOSER", "LOSES", "LOTUS",
    "LOUSE", "LOUSY", "LOVED", "LOVER", "LOVES", "LOWED", "LOWER", "LOWLY", "LOYAL", "LUCID",
    "LUCKY", "LULLS", "LUMPS", "LUMPY", "LUNAR", "LUNCH", "LUNGE", "LUNGS", "LURCH", "LURED",
    "LURES", "LURID", "LURKS", "LUSTS", "LUSTY", "LUTES", "LYING", "LYMPH", "LYNCH", "LYRIC",
    "MACES", "MADAM", "MADLY", "MAGIC", "MAIDS", "MAILS", "MAINS", "MAIZE", "MAJOR", "MAKER",
    "MAKES", "MALES", "MAMMA", "MANES", "MANGA", "MANGE", "MANGO", "MANGY", "MANIA", "MANLY",
    "MANNA", "MANOR", "MANSE", "MAPLE", "MARCH", "MARES", "MARKS", "MARRY", "MARSH", "MARTS",
    "MASKS", "MASON", "MASTS", "MATCH", "MATED", "MATES", "MAUVE", "MAXIM", "MAYBE", "MAYOR",
    "MAZES", "MEALS", "MEALY", "MEANS", "MEANT", "MEATS", "MEDAL", "MEDIA", "MEETS", "MELON",
    "MELTS", "MEMES", "MENDS", "MENUS", "MERCY", "MERES", "MERGE", "MERIT", "MERRY", "MESAS",
    "METAL", "METED", "METER", "MEWED", "MIDST", "MIENS", "MIGHT", "MILCH", "MILES", "MILKY",
    "MILLS", "MIMES", "MIMIC", "MINCE", "MINDS", "MINED", "MINER", "MINES", "MINOR", "MINTS",
    "MINUS", "MIRTH", "MISER", "MISTS", "MITES", "MIXED", "MIXES", "MOANS", "MOATS", "MOCKS",
    "MODEL", "MODEM", "MODES", "MOIST", "MOLAR", "MOLES", "MOMMA", "MONEY", "MONKS", "MONTH",
    "MOODS", "MOODY", "MOONS", "MOORS", "MOOSE", "MOPED", "MORAL", "MORES", "MOSSY", "MOTES",
    "MOTHS", "MOTIF", "MOTOR", "MOTTO", "MOUND", "MOUNT", "MOURN", "MOUSE", "MOUTH", "MOVED",
    "MOVER", "MOVES", "MOVIE", "MOWED", "MOWER", "MUCUS", "MUDDY", "MULES", "MULTI", "MUMMY",
    "MUMPS", "MUNCH", "MURAL", "MURKY", "MUSED", "MUSES", "MUSIC", "MUSKY", "MUSTY", "MUTED",
    "MUTES", "MYRRH", "MYTHS", "NABOB", "NAILS", "NAIVE", "NAKED", "NAMED", "NAMES", "NASAL",
    "NASTY", "NATAL", "NATTY", "NAVAL", "NAVEL", "NAVES", "NEARS", "NECKS", "NEEDS", "NEEDY",
    "NEIGH", "NERVE", "NESTS", "NEVER", "NEWER", "NEWLY", "NICER", "NICHE", "NIECE", "NIGHT",
    "NINNY", "NOBLE", "NOBLY", "NOISE", "NOISY", "NOMAD", "NONCE", "NOOKS", "NOOSE", "NORTH",
    "NOSED", "NOSES", "NOTCH", "NOTED", "NOTES", "NOUNS", "NOVEL", "NUDGE", "NURSE", "NYMPH",
    "OAKEN", "OAKUM", "OASES", "OASIS", "OATEN", "OATHS", "OBESE", "OBEYS", "OCCUR", "OCEAN",
    "OCHRE", "ODDER", "ODDLY", "ODIUM", "OFFAL", "OFFER", "OFTEN", "OILED", "OLDEN", "OLDER",
    "OMENS", "OMITS", "ONION", "ONSET", "OOZED", "OOZES", "OPALS", "OPENS", "OPERA", "OPINE",
    "OPIUM", "OPTIC", "ORBIT", "ORDER", "ORGAN", "OSIER", "OTHER", "OTTER", "OUGHT", "OUNCE",
    "OUTDO", "OUTER", "OVALS", "OVARY", "OVENS", "OVERT", "OWING", "OWNED", "OWNER", "OXIDE",
    "OZONE", "PACES", "PACKS", "PADDY", "PADRE", "PAEAN", "PAGAN", "PAGES", "PAILS", "PAINS",
    "PAINT", "PAIRS", "PALED", "PALER", "PALES", "PALMS", "PALMY", "PALSY", "PANEL", "PANES",
    "PANGS", "PANIC", "PANSY", "PANTS", "PAPAL", "PAPAS", "PAPER", "PARED", "PARKA", "PARKS",
    "PARRY", "PARSE", "PARTS", "PARTY", "PASHA", "PASTE", "PASTY", "PATCH", "PATES", "PATHS",
    "PATIO", "PAUSE", "PAVED", "PAWED", "PAWNS", "PAYED", "PAYER", "PEACE", "PEACH", "PEAKS",
    "PEALS", "PEARL", "PEARS", "PEASE", "PECKS", "PEDAL", "PEEPS", "PEERS", "PELTS", "PENAL",
    "PENCE", "PENIS", "PENNY", "PEONS", "PERCH", "PERIL", "PESKY", "PESOS", "PESTS", "PETAL",
    "PETTY", "PHASE", "PHIAL", "PHONE", "PHOTO", "PIANO", "PICKS", "PIECE", "PIERS", "PIETY",
    "PIGMY", "PIKES", "PILED", "PILES", "PILLS", "PILOT", "PINCH", "PINED", "PINES", "PINKS",
    "PINTO", "PINTS", "PIOUS", "PIPED", "PIPER", "PIPES", "PIQUE", "PITCH", "PITHY", "PIVOT",
    "PLACE", "PLAID", "PLAIN", "PLAIT", "PLANE", "PLANK", "PLANS", "PLANT", "PLATE", "PLAYS",
    "PLAZA", "PLEAD", "PLEAS", "PLIED", "PLIES", "PLOTS", "PLUCK", "PLUGS", "PLUMB", "PLUME",
    "PLUMS", "PLUSH", "PODIA", "POEMS", "POESY", "POETS", "POINT", "POISE", "POKED", "POKER",
    "POKES", "POLAR", "POLES", "POLKA", "POLLS", "PONDS", "POOLS", "POPES", "POPPA", "POPPY",
    "PORCH", "PORED", "PORES", "PORTS", "POSED", "POSER", "POSES", "POSSE", "POSTS", "POUCH",
    "POUND", "POURS", "POWER", "PRANK", "PRATE", "PRAYS", "PRESS", "PREYS", "PRICE", "PRICK",
    "PRIDE", "PRIED", "PRIES", "PRIME", "PRINT", "PRIOR", "PRISM", "PRIVY", "PRIZE", "PROBE",
    "PRONE", "PROOF", "PROPS", "PROSE", "PROSY", "PROUD", "PROVE", "PROWL", "PROWS", "PROXY",
    "PRUDE", "PRUNE", "PSALM", "PSHAW", "PUDGY", "PUFFS", "PUFFY", "PULLS", "PULPY", "PULSE",
    "PUMPS", "PUNCH", "PUPIL", "PUPPY", "PUREE", "PURER", "PURGE", "PURSE", "PUSSY", "PUTTY",
    "QUACK", "QUAFF", "QUAIL", "QUAKE", "QUALM", "QUART", "QUASI", "QUAYS", "QUEEN", "QUEER",
    "QUELL", "QUERY", "QUEST", "QUEUE", "QUICK", "QUIET", "QUILL", "QUILT", "QUIPS", "QUIRE",
    "QUITE", "QUITS", "QUOTA", "QUOTE", "QUOTH", "RABBI", "RABID", "RACED", "RACER", "RACES",
    "RACKS", "RADII", "RADIO", "RAFTS", "RAGED", "RAGES", "RAIDS", "RAILS", "RAINS", "RAINY",
    "RAISE", "RAJAH", "RAKED", "RAKES", "RALLY", "RANCH", "RANGE", "RANKS", "RAPID", "RARER",
    "RARES", "RATED", "RATES", "RATIO", "RAVED", "RAVEN", "RAVES", "RAYON", "RAZED", "RAZOR",
    "REACH", "REACT", "READS", "READY", "REALM", "REALS", "REAMS", "REAPS", "REARS", "REBEL",
    "REBUS", "REBUT", "RECUR", "REEDS", "REEDY", "REEFS", "REEKS", "REELS", "REEVE", "REFER",
    "REFIT", "REGAL", "REIGN", "REINS", "RELAX", "RELAY", "RELIC", "REMIT", "RENDS", "RENEW",
    "RENTS", "REPAY", "REPEL", "REPLY", "RESET", "RESIN", "RESTS", "REVEL", "REVUE", "RHEUM",
    "RHYME", "RICKS", "RIDER", "RIDES", "RIDGE", "RIFLE", "RIFTS", "RIGHT", "RIGID", "RILED",
    "RILLS", "RIMES", "RINGS", "RINSE", "RIOTS", "RIPEN", "RIPER", "RISEN", "RISER", "RISES",
    "RISKS", "RISKY", "RITES", "RIVAL", "RIVEN", "RIVER", "RIVET", "ROADS", "ROAMS", "ROARS",
    "ROAST", "ROBED", "ROBES", "ROBIN", "ROCKS", "ROCKY", "ROGUE", "ROLES", "ROLLS", "ROMAN",
    "ROOFS", "ROOKS", "ROOMS", "ROOMY", "ROOST", "ROOTS", "ROPED", "ROPES", "ROSES", "ROSIN",
    "ROUGE", "ROUGH", "ROUND", "ROUSE", "ROUTE", "ROUTS", "ROVED", "ROVER", "ROWDY", "ROWED",
    "ROYAL", "RUDER", "RUFFS", "RUINS", "RULED", "RULER", "RULES", "RUNES", "RUNGS", "RUPEE",
    "RURAL", "RUSES", "SABLE", "SABRE", "SACKS", "SADLY", "SAFER", "SAGAS", "SAGES", "SAHIB",
    "SAILS", "SAINT", "SAITH", "SALAD", "SALES", "SALLY", "SALON", "SALSA", "SALTS", "SALTY",
    "SALVE", "SALVO", "SANDS", "SANDY", "SANER", "SATED", "SATIN", "SATYR", "SAUCE", "SAUCY",
    "SAVED", "SAVES", "SAWED", "SCALD", "SCALE", "SCALP", "SCALY", "SCAMP", "SCANS", "SCANT",
    "SCARE", "SCARF", "SCARS", "SCENE", "SCENT", "SCION", "SCOFF", "SCOLD", "SCOOP", "SCOPE",
    "SCORE", "SCORN", "SCOUR", "SCOUT", "SCOWL", "SCRAP", "SCREW", "SCRIP", "SCRUB", "SCULL",
    "SEALS", "SEAMS", "SEAMY", "SEATS", "SECTS", "SEDAN", "SEDGE", "SEEDS", "SEEDY", "SEEKS",
    "SEEMS", "SEERS", "SEIZE", "SELLS", "SEMEN", "SENDS", "SENSE", "SERFS", "SERGE", "SERUM",
    "SERVE", "SEVEN", "SEVER", "SEWED", "SEWER", "SEXES", "SHACK", "SHADE", "SHADY", "SHAFT",
    "SHAKE", "SHAKY", "SHALE", "SHALL", "SHALT", "SHAME", "SHAMS", "SHANK", "SHAPE", "SHARE",
    "SHARK", "SHARP", "SHAVE", "SHAWL", "SHEAF", "SHEAR", "SHEDS", "SHEEN", "SHEEP", "SHEER",
    "SHEET", "SHEIK", "SHELF", "SHELL", "SHIED", "SHIFT", "SHINE", "SHINS", "SHINY", "SHIPS",
    "SHIRE", "SHIRK", "SHIRT", "SHOAL", "SHOCK", "SHOES", "SHONE", "SHOOK", "SHOON", "SHOOT", 
    "SHOPS", "SHORE", "SHORN", "SHORT", "SHOTS", "SHOUT", "SHOVE", "SHOWN", "SHOWS", "SHOWY",
    "SHRED", "SHREW", "SHRUB", "SHRUG", "SHUNS", "SHUTS", "SHYLY", "SIBYL", "SIDED", "SIDES",
    "SIEGE", "SIEVE", "SIGHS", "SIGHT", "SIGMA", "SIGNS", "SILKS", "SILKY", "SILLS", "SILLY",
    "SINCE", "SINEW", "SINGE", "SINGS", "SINKS", "SIREN", "SIRES", "SITES", "SIXES", "SIXTH",
    "SIXTY", "SIZED", "SIZES", "SKATE", "SKEIN", "SKIES", "SKIFF", "SKILL", "SKIMS", "SKINS",
    "SKIPS", "SKIRT", "SKULK", "SKULL", "SKUNK", "SLABS", "SLACK", "SLAGS", "SLAIN", "SLAKE",
    "SLANG", "SLANT", "SLAPS", "SLASH", "SLATE", "SLATS", "SLAVE", "SLAYS", "SLEDS", "SLEEK",
    "SLEEP", "SLEET", "SLEPT", "SLICE", "SLICK", "SLIDE", "SLILY", "SLIME", "SLIMY", "SLING",
    "SLINK", "SLIPS", "SLITS", "SLOOP", "SLOPE", "SLOPS", "SLOTH", "SLUGS", "SLUMP", "SLUMS",
    "SLUNG", "SLUNK", "SLUSH", "SLYLY", "SMACK", "SMALL", "SMART", "SMASH", "SMEAR", "SMELL",
    "SMELT", "SMILE", "SMIRK", "SMITE", "SMITH", "SMOCK", "SMOKE", "SMOKY", "SMOTE", "SNACK",
    "SNAGS", "SNAIL", "SNAKE", "SNAKY", "SNAPS", "SNARE", "SNARL", "SNEAK", "SNEER", "SNIFF",
    "SNIPE", "SNOBS", "SNORE", "SNORT", "SNOUT", "SNOWS", "SNOWY", "SNUFF", "SOAPY", "SOARS",
    "SOBER", "SOCKS", "SOFAS", "SOGGY", "SOILS", "SOLAR", "SOLES", "SOLID", "SOLOS", "SOLVE",
    "SONGS", "SONNY", "SOOTH", "SOOTY", "SORES", "SORRY", "SORTS", "SOUGH", "SOULS", "SOUND",
    "SOUPS", "SOUSE", "SOUTH", "SOWED", "SOWER", "SPACE", "SPADE", "SPAKE", "SPANK", "SPANS",
    "SPARE", "SPARK", "SPARS", "SPASM", "SPAWN", "SPEAK", "SPEAR", "SPECK", "SPEED", "SPELL",
    "SPELT", "SPEND", "SPENT", "SPERM", "SPICE", "SPICY", "SPIED", "SPIES", "SPIKE", "SPILL",
    "SPILT", "SPINE", "SPINS", "SPINY", "SPIRE", "SPITE", "SPITS", "SPLIT", "SPOIL", "SPOKE",
    "SPOOK", "SPOOL", "SPOON", "SPOOR", "SPORE", "SPORT", "SPOTS", "SPOUT", "SPRAY", "SPREE",
    "SPRIG", "SPUNK", "SPURN", "SPURS", "SPURT", "SQUAD", "SQUAT", "SQUAW", "STABS", "STACK",
    "STAFF", "STAGE", "STAGS", "STAID", "STAIN", "STAIR", "STAKE", "STALE", "STALK", "STALL",
    "STAMP", "STAND", "STANK", "STARE", "STARK", "STARS", "START", "STATE", "STAVE", "STAYS",
    "STEAD", "STEAK", "STEAL", "STEAM", "STEED", "STEEL", "STEEP", "STEER", "STEMS", "STEPS",
    "STERN", "STEWS", "STICK", "STIFF", "STILE", "STILL", "STING", "STINK", "STINT", "STIRS",
    "STOCK", "STOIC", "STOLE", "STONE", "STONY", "STOOD", "STOOL", "STOOP", "STOPS", "STORE",
    "STORK", "STORM", "STORY", "STOUT", "STOVE", "STRAP", "STRAW", "STRAY", "STREW", "STRIP",
    "STRUT", "STUCK", "STUDS", "STUDY", "STUFF", "STUMP", "STUNG", "STUNT", "STYLE", "SUAVE",
    "SUCKS", "SUGAR", "SUING", "SUITE", "SUITS", "SULKS", "SULKY", "SULLY", "SUNNY", "SUPER",
    "SURER", "SURGE", "SURLY", "SWAIN", "SWAMP", "SWANS", "SWARD", "SWARM", "SWAYS", "SWEAR",
    "SWEAT", "SWEEP", "SWEET", "SWELL", "SWEPT", "SWIFT", "SWILL", "SWIMS", "SWINE", "SWING",
    "SWIRL", "SWISH", "SWOON", "SWOOP", "SWORD", "SWORE", "SWORN", "SWUNG", "SYNOD", "SYRUP",
    "TABBY", "TABLE", "TABOO", "TACIT", "TACKS", "TAILS", "TAINT", "TAKEN", "TAKES", "TALES",
    "TALKS", "TALLY", "TALON", "TAMED", "TAMER", "TANKS", "TAPER", "TAPES", "TARDY", "TARES",
    "TARRY", "TARTS", "TASKS", "TASTE", "TASTY", "TAUNT", "TAWNY", "TAXED", "TAXES", "TEACH",
    "TEAMS", "TEARS", "TEASE", "TEEMS", "TEENS", "TEETH", "TELLS", "TEMPI", "TEMPO", "TEMPS",
    "TENDS", "TENET", "TENOR", "TENSE", "TENTH", "TENTS", "TEPEE", "TEPID", "TERMS", "TERSE",
    "TESTS", "TESTY", "TEXTS", "THANK", "THEFT", "THEIR", "THEME", "THERE", "THESE", "THICK",
    "THIEF", "THIGH", "THINE", "THING", "THINK", "THIRD", "THONG", "THORN", "THOSE", "THREE",
    "THREW", "THROB", "THROE", "THROW", "THUMB", "THUMP", "THYME", "TIARA", "TIBIA", "TICKS",
    "TIDAL", "TIDES", "TIERS", "TIGER", "TIGHT", "TILDE", "TILED", "TILES", "TILLS", "TILTS",
    "TIMED", "TIMES", "TIMID", "TINGE", "TINTS", "TIPSY", "TIRED", "TIRES", "TITHE", "TITLE",
    "TOADS", "TOAST", "TODAY", "TODDY", "TOILS", "TOKEN", "TOLLS", "TOMBS", "TOMES", "TONED",
    "TONES", "TONGS", "TONIC", "TOOLS", "TOOTH", "TOPAZ", "TOPIC", "TOQUE", "TORCH", "TORSO",
    "TORTS", "TOTAL", "TOTEM", "TOUCH", "TOUGH", "TOURS", "TOWED", "TOWEL", "TOWER", "TOWNS",
    "TOXIC", "TOYED", "TRACE", "TRACK", "TRACT", "TRADE", "TRAIL", "TRAIN", "TRAIT", "TRAMP",
    "TRAMS", "TRAPS", "TRASH", "TRAYS", "TREAD", "TREAT", "TREED", "TREES", "TREND", "TRESS",
    "TRIAD", "TRIAL", "TRIBE", "TRICE", "TRICK", "TRIED", "TRIES", "TRILL", "TRIPE", "TRIPS",
    "TRITE", "TROLL", "TROOP", "TROTH", "TROTS", "TROUT", "TRUCE", "TRUCK", "TRUER", "TRULY",
    "TRUMP", "TRUNK", "TRUSS", "TRUST", "TRUTH", "TRYST", "TUBES", "TUFTS", "TULIP", "TULLE",
    "TUNED", "TUNES", "TUNIC", "TURNS", "TUSKS", "TUTOR", "TWAIN", "TWANG", "TWEED", "TWICE",
    "TWIGS", "TWINE", "TWINS", "TWIRL", "TWIST", "TYING", "TYPED", "TYPES", "UDDER", "ULCER",
    "ULTRA", "UNCLE", "UNCUT", "UNDER", "UNDID", "UNDUE", "UNFIT", "UNION", "UNITE", "UNITS",
    "UNITY", "UNSAY", "UNTIE", "UNTIL", "UPPER", "UPSET", "URBAN", "URGED", "URGES", "URINE",
    "USAGE", "USERS", "USHER", "USING", "USUAL", "USURP", "USURY", "UTTER", "VAGUE", "VALES",
    "VALET", "VALID", "VALUE", "VALVE", "VANES", "VAPID", "VASES", "VAULT", "VAUNT", "VEILS",
    "VEINS", "VELDT", "VENAL", "VENOM", "VENTS", "VENUE", "VERBS", "VERGE", "VERSE", "VERVE",
    "VESTS", "VEXED", "VEXES", "VIALS", "VICAR", "VICES", "VIDEO", "VIEWS", "VIGIL", "VILER",
    "VILLA", "VINES", "VIOLA", "VIPER", "VIRUS", "VISIT", "VISOR", "VISTA", "VITAL", "VIVID",
    "VIXEN", "VIZOR", "VOCAL", "VODKA", "VOGUE", "VOICE", "VOILE", "VOLTS", "VOMIT", "VOTED",
    "VOTER", "VOTES", "VOUCH", "VOWED", "VOWEL", "VYING", "WADED", "WAFER", "WAFTS", "WAGED",
    "WAGER", "WAGES", "WAGON", "WAIFS", "WAILS", "WAIST", "WAITS", "WAIVE", "WAKED", "WAKEN",
    "WAKES", "WALKS", "WALLS", "WALTZ", "WANDS", "WANED", "WANES", "WANTS", "WARDS", "WARES",
    "WARMS", "WARNS", "WARTS", "WASPS", "WASTE", "WATCH", "WATER", "WAVED", "WAVER", "WAVES",
    "WAXED", "WAXEN", "WAXES", "WEARS", "WEARY", "WEAVE", "WEDGE", "WEEDS", "WEEDY", "WEEKS",
    "WEEPS", "WEIGH", "WEIRD", "WELCH", "WELLS", "WENCH", "WHACK", "WHALE", "WHARF", "WHEAT",
    "WHEEL", "WHELP", "WHERE", "WHICH", "WHIFF", "WHILE", "WHIMS", "WHINE", "WHIPS", "WHIRL",
    "WHIRR", "WHISK", "WHIST", "WHITE", "WHOLE", "WHOOP", "WHORE", "WHOSE", "WICKS", "WIDEN",
    "WIDER", "WIDOW", "WIDTH", "WIELD", "WIGHT", "WILDS", "WILES", "WILLS", "WINCE", "WINCH",
    "WINDS", "WINDY", "WINES", "WINGS", "WINKS", "WIPED", "WIPES", "WIRED", "WIRES", "WISER",
    "WISPS", "WITCH", "WITTY", "WIVES", "WOMAN", "WOMEN", "WOODS", "WOODY", "WOOED", "WOOER",
    "WORDS", "WORDY", "WORKS", "WORLD", "WORMS", "WORRY", "WORSE", "WORST", "WORTH", "WOULD",
    "WOUND", "WRACK", "WRAPS", "WRAPT", "WRATH", "WREAK", "WRECK", "WREST", "WRING", "WRIST",
    "WRITE", "WRITS", "WRONG", "WROTE", "WROTH", "YACHT", "YARDS", "YARNS", "YAWNS", "YEARN",
    "YEARS", "YEAST", "YELLS", "YELPS", "YIELD", "YOKED", "YOKES", "YOLKS", "YOUNG", "YOURS",
    "YOUTH", "ZEBRA", "ZONES",
]

alphabet = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z']

def most_used_letters():
    '''
    Outputs how many times each letter is used in the words array.
    '''
    dicto = {}
    for i in alphabet:
        count = 0
        for word in words:
            for letter in word:
                if i.upper() == letter.upper():
                    count+=1
                    break
        dicto[i] = count
    dicto = dict(sorted(dicto.items(), key=lambda item: item[1], reverse=True))
    print("Letter | Usage")
    print("--------------")
    for k in dicto:
        print(f"{k.upper()}      | {dicto[k]}")


def list_of_valid_words(letters):
    '''
    Outputs the array of valid words that can be made with the combination of letters.
    '''
    letters = sorted(letters)
    for i, letter in enumerate(letters): # Force all letters to be capitalized
        letters[i] = letter.upper()
    legal_words = []
    for word in words:
        valid_word = True
        for letter in word:
            if letter.upper() not in letters:
                valid_word = False
                break
        if valid_word and word not in legal_words:
            legal_words.append(word)
    return legal_words


def rearrange_words_by_uniqueness(words):
    unique = [word for word in words if len(word) == len(set(word))]
    duplicates = [word for word in words if len(word) != len(set(word))]
    return unique + duplicates, len(unique)


def print_valid_words(letters):
    '''
    Prints the array of valid words that the wordle_face.c can use
    '''
    legal_words = list_of_valid_words(letters)
    for i,word in enumerate(legal_words):
        legal_words[i] = word.upper().replace("D","d")
    random.shuffle(legal_words) 
    # Just in case the watch's random function is too pseudo, better to shuffle th elist so it's less likely to always have the same starting letter
    
    legal_words, num_uniq = rearrange_words_by_uniqueness(legal_words)
            
    print("static const char _valid_letters[] = {", end='')
    for letter in letters[:-1]:
        print(f"'{letter}', ", end='')
    print(f"'{letters[-1]}'" + "};")
    print("")
    
    print(f"// Number of words found: {len(legal_words)}")
    items_per_row = 9
    i = 0
    print("static const char _legal_words[][WORDLE_LENGTH + 1] = {")
    while i < len(legal_words):
        print("    ", end='')
        for j in range(min(items_per_row, len(legal_words)-i)):
            print(f'"{legal_words[i]}", ', end='')
            i+=1
        print('')
    print("};")
    print(f"\nstatic const uint16_t _num_unique_words = {num_uniq};  // The _legal_words array begins with this many words where each letter is different.")


def get_sec_val_and_units(seconds):
    if seconds < 1:
        return f"{round(seconds * 1000)} ms"
    hours = int(seconds // 3600)
    minutes = int((seconds % 3600) // 60)
    secs = int(seconds % 60)
    if hours > 0:
        return f"{hours} hr {minutes} min {secs} sec"
    elif minutes > 0:
        return f"{minutes} min {secs} sec"
    else:
        return f"{secs} sec"


def txt_of_all_letter_combos(num_letters_in_set):
    '''
    Creates a txt file that shows every combination of letters and how many words
    their combo can make.
    num_letters_in_set - How many letters should be in each combination
    '''
    num_status_prints = 100
    dict_combos_counts = {}
    print_iter = 0
    prev = time.time()
    start = prev
    letters_to_ignore = ['D','T']  # Don't diplay well on the watch
    legal_letters = [item for item in alphabet if item not in letters_to_ignore]
    print(f"Finding all {num_letters_in_set} letter combinations with the following letters: {legal_letters}")
    all_combos = list(itertools.combinations(legal_letters, num_letters_in_set))
    len_all_combos = len(all_combos)
    to_print = max(1, int(len_all_combos/ num_status_prints))
    print(f"Amount of Combos: {len_all_combos}")
    estimated_prints = round(len_all_combos / to_print)
    for i, letters in enumerate(all_combos):
        letters = sorted(letters)
        dict_combos_counts[repr(letters)] = len(list_of_valid_words(letters))
        print_iter+=1
        if print_iter >= to_print:
            curr = time.time()
            delta = curr - prev
            time_passed = curr - start
            total_time_estimate = delta * estimated_prints
            time_left_estimate = (delta * estimated_prints) - time_passed
            output = f"Time Passed: {get_sec_val_and_units(time_passed)} | "
            output+= f"Amount of time for {to_print} items: {get_sec_val_and_units(delta)} | "
            output+= f"Estimate for total: {get_sec_val_and_units(total_time_estimate)} | "
            output+= f"items Left {len_all_combos - i} | "
            output+= f"Percent Complete {round((100 * i) / len_all_combos)}% | "
            output+= f"Estimated Time Left : {get_sec_val_and_units(time_left_estimate)}"
            print(output)
            prev = curr
            print_iter = 0
    dict_combos_counts = dict(sorted(dict_combos_counts.items(), key=lambda item: item[1], reverse=True))
    
    most_common_key = next(iter(dict_combos_counts))
    print(f"The Most Common Combo is: {most_common_key}")
    print_valid_words(ast.literal_eval(most_common_key))
    
    with open('output.txt', 'w') as file:
        for key, value in dict_combos_counts.items():
            file.write(f'{key}: {value}\n')


if __name__ == "__main__":
    #most_used_letters()
    print_valid_words(['A', 'C', 'E', 'I', 'L', 'N', 'O', 'P', 'R', 'S'])
    #txt_of_all_letter_combos(10)