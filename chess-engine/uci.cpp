#include "uci.hpp"
static void uci_send_id()
{
    std::cout << "id name " << NAME << std::endl;
    std::cout << "id author " << AUTHOR << std::endl;
    std::cout << "option name Hash type spin default 64 min 4 max " << MAXHASH << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "uciok" << std::endl;
}


int DefaultHashSize = 64;
int CurrentHashSize = DefaultHashSize;
int LastHashSize = CurrentHashSize;

bool IsUci = false;

TranspositionTable *table;

void uci_loop()
{
    SearchInfo info;
    SearchThread searchThread(info);
    searchThread.board = Board(DEFAULT_POS);
    auto ttable = std::make_unique<TranspositionTable>();
    table = ttable.get();
    table->Initialize(DefaultHashSize);

    // Create our board instance
    int default_depth = 15; // Default search depth
    std::string command;
    std::string token;

    while (std::getline(std::cin, command))
    {
        std::istringstream is(command);

        token.clear();
        is >> std::skipws >> token;

        if (token == "stop")
        {
            info.stopped = true;
        }
        else if (token == "quit")
        {
            info.stopped = true;
            break;
        }
        else if (token == "isready")
        {

            std::cout << "readyok" << std::endl;
            continue;
        }
        else if (token == "ucinewgame")
        {
            table->Initialize(CurrentHashSize);
            searchThread.applyFen(DEFAULT_POS);
            continue;
        }
        else if (token == "uci")
        {
            IsUci = true;
            uci_send_id();
                continue;
        }

        /* Handle UCI position command */
        else if (token == "position")
        {
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos")
            {
                searchThread.applyFen(DEFAULT_POS);
            }
            else if (option == "fen")
            {
                std::string final_fen;
                is >> std::skipws >> option;
                final_fen = option;

                // Record side to move
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Record castling
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record enpassant square
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record fifty move conter
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Finally, apply the fen.
                searchThread.applyFen(final_fen);
            }
            is >> std::skipws >> option;
            if (option == "moves")
            {
                std::string moveString;

                while (is >> moveString)
                {
                    searchThread.board.makeMove(convertUciToMove(searchThread.board,moveString));
                }
            }
            continue;
        }

        /* Handle UCI go command */
        else if (token == "go")
        {
            is >> std::skipws >> token;

            // Initialize variables
            int depth = default_depth;
            
            uint64_t nodes = -1;

            while (token != "none")
            {
                if (token == "infinite")
                {
                    depth = -1;
                    break;
                }
                if (token == "movestogo")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.movestogo = stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Depth
                if (token == "depth")
                {
                    is >> std::skipws >> token;
                    depth = std::stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Time
                if (token == "wtime")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.wtime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "btime")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.btime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Increment
                if (token == "winc")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.winc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "binc")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.binc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                if (token == "movetime")
                {
                    is >> std::skipws >> token;
                    searchThread.tm.movetime = stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "nodes")
                {
                    is >> std::skipws >> token;
                    nodes = stoi(token);
                    is >> std::skipws >> token;
                }
                token = "none";
            }

            if (nodes != -1)
            {
                info.nodes = nodes;
                info.nodeset = true;
            }

            info.depth = depth;
            if (searchThread.tm.wtime != -1 || searchThread.tm.btime != -1 || searchThread.tm.movetime != -1)
            {
                info.timeset = true;
            }

            if (depth == -1)
            {
                info.depth = MAXPLY;
            }

            info.stopped = false;
            info.uci = IsUci;
            iterativeDeepening(searchThread, info.depth);

        }else if (token == "setoption")
        {
            is >> std::skipws >> token;
            if (token == "name")
            {
                is >> std::skipws >> token;
                if (token == "Hash")
                {
                    is >> std::skipws >> token; // Skip "value"
                    is >> std::skipws >> token;
                    CurrentHashSize = std::stoi(token);
                    table->Initialize(CurrentHashSize);
                }
            }
        }
        /* Debugging Commands */
        else if (token == "print")
        {
            std::cout << searchThread.board << std::endl;
            continue;
        }
        else if (token == "bencheval")
        {

            long samples = 1000000000;
            long long timeSum = 0;
            int output;
            for (int i = 0; i < samples; i++)
            {
                auto start = std::chrono::high_resolution_clock::now();
                output = evaluate(searchThread.board);
                auto stop = std::chrono::high_resolution_clock::now();
                timeSum += std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            }
            auto timeAvg = (double)timeSum / samples;
            std::cout << "Output: " << output << " , Time: " << timeAvg << "ns" << std::endl;

            continue;
        }
        else if (token == "eval")
        {

            std::cout << "Eval: " << evaluate(searchThread.board) << std::endl;
        }
        else if (token == "repetition")
        {

            std::cout << searchThread.board.isRepetition() << std::endl;
        }
        else if (token == "side")
        {

            std::cout << (searchThread.board.sideToMove == White ? "White" : "Black") << std::endl;
        }
        
    }

    table->clear();

    std::cout << std::endl;
    if (!info.uci)
    {
        std::cout << "\u001b[0m";
    }
}