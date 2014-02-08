%%%-------------------------------------------------------------------
%%% File    : chat_server.erl
%%% Author  :  <apn@DEFMACRO>
%%% Description : The chat server is included for the examples in tinch++.
%%%               Here we implement a primitive broadcast chat; every message 
%%%               a client publishes is distributed to all connected clients.
%%%
%%% Created : 10 Mar 2010 by  <apn@DEFMACRO>
%%%-------------------------------------------------------------------
-module(chat_server).

-behaviour(gen_server).

%% API
-export([start_link/0, register_client/2, unregister_client/1,
	publish/2, registrations/0]).

%% gen_server callbacks
-export([init/1, handle_call/3, handle_cast/2, handle_info/2,
	 terminate/2, code_change/3]).

-define(SERVER, ?MODULE).

%%====================================================================
%% API
%%====================================================================
%%--------------------------------------------------------------------
%% Function: start_link() -> {ok,Pid} | ignore | {error,Error}
%% Description: Starts the server
%%--------------------------------------------------------------------
start_link() ->
    gen_server:start_link({local, ?SERVER}, ?MODULE, [], []).

%%--------------------------------------------------------------------
%% Function: register_client(Name, Pid) -> ok | {error,Error}
%% Description: -
%%--------------------------------------------------------------------
register_client(Name, Pid) ->
    gen_server:call(?SERVER, {register_client, Name, Pid}).

%%--------------------------------------------------------------------
%% Function: unregister_client(Pid) -> ok
%% Description: This function will not be needed in tinch++ 0.2.
%%              Once we have implemented links, registrations are 
%%              removed as a link breaks instead.
%%--------------------------------------------------------------------
unregister_client(Pid) ->
    gen_server:cast(?SERVER, {unregister_client, Pid}),
    ok.

%%--------------------------------------------------------------------
%% Function: publish(Msg, FromPid) -> ok | {error,Error}
%% Description: Published the given message on all registered chat 
%%              clients.
%%              The protocol for the clients is: {chat_msg, From, Msg}
%%--------------------------------------------------------------------
publish(Msg, FromPid) ->
    gen_server:call(?SERVER, {publish, Msg, FromPid}).

%%--------------------------------------------------------------------
%% Function: registrations() -> [Registrations]
%% Description: Returns a list of all registered clients.
%%--------------------------------------------------------------------
registrations() ->
    gen_server:call(?SERVER, registrations).

%%====================================================================
%% gen_server callbacks
%%====================================================================

%%--------------------------------------------------------------------
%% Function: init(Args) -> {ok, State} |
%%                         {ok, State, Timeout} |
%%                         ignore               |
%%                         {stop, Reason}
%% Description: Initiates the server
%%--------------------------------------------------------------------
init([]) ->
    {ok, dict:new()}.

%%--------------------------------------------------------------------
%% Function: %% handle_call(Request, From, State) -> {reply, Reply, State} |
%%                                      {reply, Reply, State, Timeout} |
%%                                      {noreply, State} |
%%                                      {noreply, State, Timeout} |
%%                                      {stop, Reason, Reply, State} |
%%                                      {stop, Reason, State}
%% Description: Handling call messages
%%--------------------------------------------------------------------
handle_call({register_client, Name, Pid}, _From, State) ->
    NewRegistrations = dict:append(Pid, Name, State),
    {reply, ok, NewRegistrations};
handle_call({publish, Msg, FromPid}, _From, State) ->
    AllRegistrations = dict:fetch_keys(State),
    Receivers = AllRegistrations -- [FromPid],
    {ok, [Publisher]} = dict:find(FromPid, State),
    ClientMsg = {chat_msg, Publisher, Msg},
    [Client ! ClientMsg || Client <- Receivers],
    {reply, ok, State};
handle_call(registrations, _From, State) ->
    Reply = dict:to_list(State),
    {reply, Reply, State}.

%%--------------------------------------------------------------------
%% Function: handle_cast(Msg, State) -> {noreply, State} |
%%                                      {noreply, State, Timeout} |
%%                                      {stop, Reason, State}
%% Description: Handling cast messages
%%--------------------------------------------------------------------
handle_cast({unregister_client, Pid}, State) ->
    RemainingRegistrations = dict:erase(Pid, State),
    {noreply, RemainingRegistrations}.

%%--------------------------------------------------------------------
%% Function: handle_info(Info, State) -> {noreply, State} |
%%                                       {noreply, State, Timeout} |
%%                                       {stop, Reason, State}
%% Description: Handling all non call/cast messages
%%--------------------------------------------------------------------
handle_info(_Info, State) ->
    {noreply, State}.

%%--------------------------------------------------------------------
%% Function: terminate(Reason, State) -> void()
%% Description: This function is called by a gen_server when it is about to
%% terminate. It should be the opposite of Module:init/1 and do any necessary
%% cleaning up. When it returns, the gen_server terminates with Reason.
%% The return value is ignored.
%%--------------------------------------------------------------------
terminate(_Reason, _State) ->
    ok.

%%--------------------------------------------------------------------
%% Func: code_change(OldVsn, State, Extra) -> {ok, NewState}
%% Description: Convert process state when code is changed
%%--------------------------------------------------------------------
code_change(_OldVsn, State, _Extra) ->
    {ok, State}.

%%--------------------------------------------------------------------
%%% Internal functions
%%--------------------------------------------------------------------
