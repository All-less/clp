"""MCP Server implementation."""

import json
from collections import defaultdict, Counter
from typing import Any

from clp_py_utils.clp_config import ClpConfig
from fastmcp import Context, FastMCP
from starlette.requests import Request
from starlette.responses import PlainTextResponse

from clp_mcp_server.clp_connector import ClpConnector

from . import constants
from .session_manager import SessionManager
from .utils import format_query_results, parse_timestamp_range, sort_by_timestamp


def create_mcp_server(clp_config: ClpConfig) -> FastMCP:
    """
    Creates and defines API tool calls for the CLP MCP server.

    :param clp_config:
    :return: A configured `FastMCP` instance.
    :raise: Propagates `FastMCP.__init__`'s exceptions.
    :raise: Propagates `FastMCP.tool`'s exceptions.
    """
    mcp = FastMCP(name=constants.SERVER_NAME)

    session_manager = SessionManager(session_ttl_seconds=constants.SESSION_TTL_SECONDS)

    connector = ClpConnector(clp_config)

    async def _execute_metadata_query(session_id: str, kql_query: str) -> dict[str, Any]:
        """
        Executes a KQL metadata query and returns the results.

        :param session_id:
        :param kql_query:
        :return: A dictionary containing the metadata results on success.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        try:
            query_id = await connector.submit_query(kql_query)
            await connector.wait_query_completion(query_id)
            results = await connector.read_metadata_results(query_id)
        except (ValueError, RuntimeError, TimeoutError) as e:
            return {"Error": str(e)}

        return {"results": results}

    async def _execute_kql_query(
        session_id: str,
        kql_query: str,
        begin_ts: int | None = None,
        end_ts: int | None = None,
    ) -> dict[str, Any]:
        """
        Executes a KQL query search with optional timestamp range and returns paginated results.

        :param session_id:
        :param kql_query:
        :param begin_ts: The beginning of the time range (inclusive).
        :param end_ts: The end of the time range (inclusive).
        :return: Forwards `SessionManager.cache_query_result_and_get_first_page`'s return values on
            success.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        try:
            query_id = await connector.submit_query(kql_query, begin_ts, end_ts)
            await connector.wait_query_completion(query_id)
            results = await connector.read_results(query_id)
        except (ValueError, RuntimeError, TimeoutError) as e:
            return {"Error": str(e)}

        sorted_results = sort_by_timestamp(results)
        formatted_results = format_query_results(sorted_results)
        return session_manager.cache_query_result_and_get_first_page(session_id, formatted_results)

    @mcp.tool
    async def get_instructions(ctx: Context) -> str:
        """
        Gets a pre-defined "system prompt" that guides the LLM behavior.
        This function must be invoked before any other `FastMCP.tool`.

        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A string of "system prompt".
        """
        await session_manager.start()
        return session_manager.get_or_create_session(ctx.session_id).get_instructions()

    @mcp.tool
    async def get_nth_page(page_index: int, ctx: Context) -> dict[str, Any]:
        """
        Retrieves the n-th page of a paginated response with the paging metadata from the previous
        query.

        :param page_index: Zero-based index, e.g., 0 for the first page.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()
        return session_manager.get_nth_page(ctx.session_id, page_index)

    @mcp.tool
    def hello_world(name: str = "clp-mcp-server user") -> dict[str, Any]:
        """
        Provides a simple hello world greeting.

        :param name:
        :return: A greeting message to the given `name`.
        """
        return {
            "message": f"Hello World, {name.strip()}!",
            "server": constants.SERVER_NAME,
            "status": "running",
        }

    @mcp.tool
    async def search_by_kql(kql_query: str, ctx: Context) -> dict[str, Any]:
        """
        Searches log events that match the given Kibana Query Language (KQL) query. The resulting
        events are ordered by timestamp in descending order (latest to oldest), cached for
        subsequent pagination, and returned with the first page of results.

        :param kql_query:
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()

        return await _execute_kql_query(ctx.session_id, kql_query)

    @mcp.tool
    async def search_by_kql_with_timestamp_range(
        kql_query: str, formatted_begin_timestamp: str, formatted_end_timestamp: str, ctx: Context
    ) -> dict[str, Any]:
        """
        Searches log events that match the given Kibana Query Language (KQL) query within the given
        time range. Timestamps must follow the ISO 8601 UTC format (`YYYY-MM-DDTHH:mm:ss.fffZ`),
        where the trailing `Z` indicates UTC. Timestamps that do not follow this format will be
        rejected.

        :param kql_query:
        :param formatted_begin_timestamp: The beginning of the time range (inclusive).
        :param formatted_end_timestamp: The end of the time range (inclusive).
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A dictionary containing the following key-value pairs on success:
            - "items": A list of log entries in the requested page.
            - "num_total_pages": Total number of pages available from the query as an integer.
            - "num_total_items": Total number of log entries available from the query as an integer.
            - "num_items_per_page": Number of log entries per page.
            - "has_next": Whether a page exists after the returned one.
            - "has_previous": Whether a page exists before the returned one.
        :return: A dictionary with the following key-value pair on failures:
            - "Error": An error message describing the failure.
        """
        await session_manager.start()

        try:
            begin_ts, end_ts = parse_timestamp_range(
                formatted_begin_timestamp, formatted_end_timestamp
            )
        except ValueError as e:
            return {"Error": str(e)}

        return await _execute_kql_query(ctx.session_id, kql_query, begin_ts, end_ts)

    def flatten_dict(d, parent_key='', sep='.'):
        items = []
        for k, v in d.items():
            new_key = parent_key + sep + k if parent_key else k
            if isinstance(v, dict):
                items.extend(flatten_dict(v, new_key, sep=sep).items())
            else:
                items.append((new_key, v))
        return dict(items)

    def access(doc, keys):
        if not keys:
            return doc
        elif keys[0] in doc:
            return access(doc[keys[0]], keys[1:])
        else:
            return None

    def gen_counter(iterable):
        res = {}
        for item in iterable:
            if item in res:
                res[item] += 1
            else:
                res[item] = 1
        return res

    def group_by_vars(docs):
        flattened_docs = []
        for doc in docs:
            doc['message'] = json.loads(doc['message'])
            processed_doc = {
                "LogType": doc['message']['msg']['LogType'],
                "Timestamp": doc['message']['timestamp']
            }
            del doc['message']['msg']['LogType']
            del doc['message']['timestamp']
            processed_doc["Variables"] = flatten_dict(doc['message'])
            flattened_docs.append(processed_doc)

        group_by_logtype = defaultdict(list)
        for doc in flattened_docs:
            group_by_logtype[doc['LogType']].append(doc)

        grouped_result = {}
        for logType, docs in group_by_logtype.items():
            grouped_result[logType] = {
                "Timestamps": [doc['Timestamp'] for doc in docs],
                "Repetitions": len(docs),
                "GroupBy": {}
            }
            
            variable_counter = defaultdict(Counter)
            for doc in docs:
                for var_name, var_value in doc['Variables'].items():
                    variable_counter[var_name][var_value] += 1
            
            grouped_result[logType]["GroupBy"] = dict(variable_counter)
        
        return [ { "LogType": k, **v } for k, v in grouped_result.items() ]

    def group_by_key(docs, groupby):
        if '.' in groupby:
            fields = groupby.split('.')
        else:
            fields = [groupby]
                
        filtered_result = []
        for doc in docs:
            doc['message'] = json.loads(doc['message'])
            if (val := access(doc['message'], fields)) is not None:
                filtered_result.append({
                    "LogType": doc['message']['msg']['LogType'],
                    "Value": val,
                    "Timestamp": doc['message']['timestamp']
                })

        group_by_logtype = defaultdict(list)
        for doc in filtered_result:
            group_by_logtype[doc['LogType']].append(doc)

        grouped_result = {}
        for logType, docs in group_by_logtype.items():
            grouped_result[logType] = {
                "Timestamps": [doc['Timestamp'] for doc in docs],
                "Repetitions": len(docs),
                "GroupBy": {
                    groupby: Counter([doc['Value'] for doc in docs])
                }
            }
        
        return [ { "LogType": k, **v } for k, v in grouped_result.items() ]

    @mcp.tool
    async def search_and_group_by(kql_query: str, groupby: str, ctx: Context):
        """
        Searches log events that match the given Kibana Query Language (KQL) query and groups
        the results by the specified variable or all variables.

        :param kql_query: The KQL query string to search for within log events.
        :param groupby: The variable name to group by, or '*' to group by all variables. The variable
        name can be nested using dot notation (e.g., 'field1.field2').
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP session.
        :return: A list of grouped results, where each group contains the log type, timestamps,
            repetitions, and grouped variable counts.
        :raise: Exception if any error occurs during processing.
        """
        await session_manager.start()

        # TODO(refactor): here should be _execute_kql_query but it has caching. 
        query_result = await _execute_metadata_query(ctx.session_id, kql_query)
        if "Error" in query_result:
            return query_result

        try:
            if groupby == '*':
                return group_by_vars(query_result['results'])
            else:
                return group_by_key(query_result['results'], groupby)
        except Exception as e:
            return {"Error": str(e)}


    @mcp.tool
    async def search_and_output_logtypes(kql_query: str, ctx: Context):
        """
        Retrieves all log types that match the given Kibana Query Language (KQL) query.
        :param kql_query: The KQL query string to search for within log types.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP
        ession.
        """
        await session_manager.start()

        query_result = await _execute_metadata_query(ctx.session_id, "stats.logtypes")
        if "Error" in query_result:
            return query_result

        result = []
        for logtype_doc in query_result["results"]:
            try:
                doc = json.loads(logtype_doc["message"])
                if kql_query in doc.get("logtype", ""):
                    result.append(doc)
            except json.JSONDecodeError:
                if kql_query in logtype_doc.get("message", ""):
                    result.append(logtype_doc.get("message"))
        
        return result
        

    @mcp.tool
    async def get_variable_values_with_counts(variable_name: str, ctx: Context):
        """
        Retrieves all values and their counts for the specified variable.
        :param variable_name: The name of the variable to retrieve values for.
        :param ctx: The `FastMCP` context containing the metadata of the underlying MCP
        """
        await session_manager.start()

        query_result = await _execute_metadata_query(ctx.session_id, "stats.variables")
        if "Error" in query_result:
            return query_result
        
        result = []
        for var_doc in query_result["results"]:
            try:
                doc = json.loads(var_doc["message"])
                if variable_name == doc.get("type", ""):
                    result.append(doc)
            except json.JSONDecodeError:
                if variable_name in var_doc.get("message", ""):
                    result.append(var_doc.get("message", ""))
        
        return result

    @mcp.custom_route("/health", methods=["GET"])
    async def health_check(_request: Request) -> PlainTextResponse:
        """
        Health check endpoint.

        :param _request: An HTTP request object.
        :return: A plain text response indicating server is healthy.
        """
        return PlainTextResponse("OK")

    return mcp
